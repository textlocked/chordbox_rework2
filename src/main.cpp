// ChordBox rework — Stage 1 (clean rebuild)
//
// Goal for this stage, and ONLY this stage:
//   1. Bring up the Pico's own USB-C port as a native USB-MIDI host,
//      using TinyUSB's built-in tuh_midi_* driver directly — no
//      usb_midi_host, no EZ_USB_MIDI_HOST, no Arduino MIDI Library.
//   2. Track every held note.
//   3. Print the held notes (plural) to the ST7789 TFT, since the USB
//      port is busy hosting the keyboard and Serial-over-USB is gone.
//
// The tuh_midi_* function/struct shapes below are copied from the
// ACTUAL installed header on this machine (verified by hand, not
// reconstructed from a changelog):
//   .../Adafruit_TinyUSB_Arduino/src/class/midi/midi_host.h
//
// We use the raw 4-byte Packet API (tuh_midi_packet_read), not the
// Stream API — it needs no extra config flag and is enough for
// Note On/Off, which is all this stage cares about. Each USB-MIDI
// packet is a fixed 4-byte layout per the USB MIDI 1.0 spec:
//   packet[0] = (cable_num << 4) | code_index_number
//   packet[1] = MIDI status byte   (e.g. 0x90 = Note On, ch 0)
//   packet[2] = data byte 1        (note number)
//   packet[3] = data byte 2        (velocity)

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Adafruit_TinyUSB.h> // bundled with the earlephilhower core
#include "pico/critical_section.h" // real pico-sdk primitive, bundled with the core

#include <box_draw.h> // text box custom helpers :3 
#include <chord_detector.h>

#include "Helvetica7pt7b.h"
#include "Helvetica9pt7b.h"
#include "Helvetica12pt7b.h" 
#include "Helvetica18pt7b.h"
#include "Helvetica20pt7b.h" 
#include "Helvetica24pt7b.h" 
#include "Helvetica44pt7b.h" 
#include "Helvetica_Bold7pt7b.h"
#include "Helvetica_Bold9pt7b.h"
#include "Helvetica_Bold12pt7b.h"
#include "Helvetica_Bold18pt7b.h"
#include "Helvetica_Bold20pt7b.h"
#include "Helvetica_Bold24pt7b.h" 
#include "Helvetica_Bold44pt7b.h"

// ---------------------------------------------------------------------
// TFT wiring
// ---------------------------------------------------------------------
// SCK=18 and MOSI=19 are the Pico's *default* hardware SPI0 pins in the
// earlephilhower core, so &SPI (=SPI0) can be handed straight to the
// display driver with no manual pin remapping.
#define TFT_CS   17
#define TFT_DC   20
#define TFT_RST  21
#define TFT_BLK  22

#define UART1_TX_PIN 0  // Connects to CH340 White (RXD)
#define UART1_RX_PIN 1  // Connects to CH340 Green (TXD)

// ---------------------------------------------------------------------
// USB MIDI host (native RP2040 host hardware, no Pico-PIO-USB)
// ---------------------------------------------------------------------
// This lives on core0 only. All of core0's setup()/loop() is now kept
// to just USB servicing — display, chord detection (later), and any
// other UI work belongs on core1 via setup1()/loop1(), so it can never
// delay the next incoming MIDI transfer.
Adafruit_USBH_Host USBHost;

static uint8_t midiIdx = 0xFF; // TinyUSB "interface index", not device address; 0xFF = none mounted

// ---------------------------------------------------------------------
// Note tracking — the shared state between core0 (writer) and core1
// (reader, once chord detection/UI code lives there)
// ---------------------------------------------------------------------
// activeNotes[] itself is ONLY ever written from core0 (inside
// tuh_midi_rx_cb). Anything on core1 that needs to know which notes are
// currently held must go through snapshotActiveNotes() below rather
// than reading activeNotes[] directly — a chord detector reading the
// raw array mid-update could see a torn snapshot (e.g. 3 of 4 notes in
// a fast chord already updated, 1 not yet), which self-corrects within
// microseconds but could still occasionally misidentify a chord for an
// instant. critical_section_t is a real RP2040 hardware spinlock from
// the pico-sdk (bundled with this core), not something improvised —
// cheap enough (sub-microsecond) to use around every access without
// itself becoming a bottleneck.
static bool activeNotes[128] = { false };
static volatile bool notesDirty = true; // force first draw
static critical_section_t noteLock;

// Call once, from core0's setup(), before core1 starts touching
// anything. Safe to call snapshotActiveNotes() from core1 after that.
void initNoteLock() {
    critical_section_init(&noteLock);
}

// Core1 (or anywhere off the USB-callback path) should call this to get
// a consistent, non-torn copy of which notes are currently held.
void snapshotActiveNotes(bool out[128]) {
    critical_section_enter_blocking(&noteLock);
    memcpy(out, (const void*)activeNotes, sizeof(activeNotes));
    critical_section_exit(&noteLock);
}

const char* NOTE_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

String noteName(uint8_t note) {
    int octave = (note / 12) - 1; // MIDI note 60 = C4
    return String(NOTE_NAMES[note % 12]) + String(octave);
}

// ---------------------------------------------------------------------
// Fast in-RAM diagnostic log
// ---------------------------------------------------------------------
// Pure memory writes only in here — no Serial calls. The goal is to see
// real microsecond-level timing of what tuh_midi_rx_cb actually receives
// during a fast burst, without the act of logging changing that timing
// (which is exactly what happened when Serial1.print lived in the
// callback before). The buffer gets dumped to Serial1 later, from
// loop(), once things go quiet.
struct MidiLogEntry {
    uint32_t t_us;
    uint8_t  packet[4];
};
static const int MIDI_LOG_SIZE = 64;
static MidiLogEntry midiLog[MIDI_LOG_SIZE];
static volatile uint16_t midiLogCount = 0;
static volatile uint32_t midiLogLastEventMs = 0;
static bool midiLogPendingDump = false;

// ---------------------------------------------------------------------
// TinyUSB MIDI host callbacks
// ---------------------------------------------------------------------
extern "C" {

void tuh_midi_mount_cb(
    uint8_t idx,
    const tuh_midi_mount_cb_t* data)
{
    midiIdx = idx;

    Serial1.println("MIDI MOUNT");

    if (data)
    {
        Serial1.print("RX cables: ");
        Serial1.println(data->rx_cable_count);

        Serial1.print("TX cables: ");
        Serial1.println(data->tx_cable_count);
    }

    notesDirty = true;
}

void tuh_midi_umount_cb(uint8_t idx) {
    if (idx == midiIdx) {
        midiIdx = 0xFF;
        memset(activeNotes, 0, sizeof(activeNotes));
        notesDirty = true;
    }
}

// Fired when new MIDI data has arrived. Drain every full 4-byte packet
// available right now, so we don't fall behind before the next callback.
//
// NOTE: this only touches activeNotes[] and the fast in-RAM log above —
// no Serial calls, no drawing. That work happens later, in loop().
void tuh_midi_rx_cb(uint8_t idx, uint32_t num_packets)
{
    if (idx != midiIdx)
        return;

    uint8_t packet[4];

    while (tuh_midi_packet_read(idx, packet))
    {
        if (midiLogCount < MIDI_LOG_SIZE) {
            midiLog[midiLogCount].t_us = micros();
            memcpy(midiLog[midiLogCount].packet, packet, 4);
            midiLogCount++;
        }
        midiLogLastEventMs = millis();

        uint8_t status = packet[1];

        if ((status & 0xF0) == 0x90)
        {
            critical_section_enter_blocking(&noteLock);
            activeNotes[packet[2] & 0x7F] = packet[3] != 0;
            critical_section_exit(&noteLock);
            notesDirty = true;
        }
        else if ((status & 0xF0) == 0x80)
        {
            critical_section_enter_blocking(&noteLock);
            activeNotes[packet[2] & 0x7F] = false;
            critical_section_exit(&noteLock);
            notesDirty = true;
        }
    }
}

} // extern "C"

// ---------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------
// Tracks how tall the previous frame's text block was, so the next
// clear only needs to cover the taller of "what's there now" and
// "what's about to be drawn" — not the whole 170px-tall panel.
static int lastUsedHeight = 0;

void drawNotes() {
    bool notes[128];
    snapshotActiveNotes(notes);

    tft.setTextSize(1);
    const int lineHeight = 20;
    const int maxWidth   = 320 - 8; // 8px margin

    if (midiIdx == 0xFF) {
        int clearHeight = max(lastUsedHeight, lineHeight + 4);
        tft.fillRect(0, 0, 320, clearHeight, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setFreeFont(&Helvetica7pt7b);

        tft.print("Waiting for MIDI...");
        lastUsedHeight = lineHeight + 4;
        return;
    }

    // Pass 1: figure out how much vertical space this frame needs,
    // without drawing anything yet.
    int dryX = 4, dryY = 4;
    for (int n = 0; n < 128; n++) {
        if (!notes[n]) continue;
        int w = (noteName(n).length() + 1) * 12;
        if (dryX + w > maxWidth) {
            dryX = 4;
            dryY += lineHeight;
            if (dryY > 170 - lineHeight) break;
        }
        dryX += w;
    }
    int neededHeight = dryY + lineHeight;

    // Clear only the taller of "what's there now" and "what's about to
    // be drawn" — not the whole panel.
    tft.fillRect(0, 0, 320, max(lastUsedHeight, neededHeight), TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    lastUsedHeight = neededHeight;

    // Pass 2: actually draw.
    int cursorX = 4;
    int cursorY = 4;
    int shown = 0;

    for (int n = 0; n < 128; n++) {
        if (!notes[n]) continue;

        String label = noteName(n) + " ";
        int w = label.length() * 12; // ~12px per glyph at text size 2

        if (cursorX + w > maxWidth) {
            cursorX = 4;
            cursorY += lineHeight;
            if (cursorY > 170 - lineHeight) break; // out of vertical room
        }

        tft.setCursor(cursorX, cursorY);
        tft.print(label);
        cursorX += w;
        shown++;
    }

    if (shown == 0) {
        tft.setCursor(4, 4);
        tft.print("(no notes held)");
    }
}

// ---------------------------------------------------------------------
// core0: setup() / loop() — USB servicing ONLY
// ---------------------------------------------------------------------
// Nothing here touches the TFT, does Serial I/O, or does anything else
// that could take an unpredictable amount of time. The entire point of
// the core split is that this loop should be doing essentially nothing
// but USBHost.task() as fast as it possibly can.
//
// Ordering note: core0 and core1 start close to simultaneously with no
// guaranteed order between them, so noteLock has to be initialized
// before EITHER core can touch it. Rather than have core0 wait on
// core1 (which would just be cosmetic — nothing about USB bring-up
// actually depends on the screen), core1 waits on lockReady, which
// core0 sets immediately after initNoteLock().
static volatile bool lockReady = false;

void setup() {
    
    initNoteLock();
    lockReady = true;

    USBHost.begin(0); // rhport 0 = the Pico's native USB-C port, host mode
}

void loop()
{
    USBHost.task();
}

// ---------------------------------------------------------------------
// core1: setup1() / loop1() — display + diagnostics
// ---------------------------------------------------------------------
// Everything that isn't strictly "receive MIDI as fast as possible"
// lives here from now on: the TFT, Serial1 debug output, and — later —
// chord detection, the rotary encoder, etc. None of it can delay core0
// servicing the USB stack, because it's physically running on the other
// core.
void setup1() {
    while (!lockReady) { /* spin — noteLock must exist before drawNotes() can call snapshotActiveNotes() */ }
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, LOW);

    
    
    Serial1.setTX(UART1_TX_PIN);
    Serial1.setRX(UART1_RX_PIN);
    Serial1.begin(115200);

    tft.init();
    tft.setRotation(1); // landscape, 320x170
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    drawScreen( // First drawScreen() is a refrence of placeholder text. If there's no data for each box, then you may use the placeholders here instead.
            "KEY: C / Am", 
            "--",
            "--",
            "- - -",
            nullptr,
            0
        );

    digitalWrite(TFT_BLK, HIGH);
}

void loop1()
{
    String heldNotes;
    bool firstNote = true;
    bool notes[128];
    // Wait for a short quiet gap since the last MIDI byte before actually
    // redrawing. Without this, a fast 4-note burst meant 4 separate
    // screen writes interleaved with USB servicing — one right after
    // each note — which eats into the time available to service the
    // next incoming transfer. 15ms is imperceptible for a chord display
    // but enough to let a whole burst land before we draw it once.

    const uint32_t DRAW_DEBOUNCE_MS = 33;
    if (notesDirty && (millis() - midiLogLastEventMs) > DRAW_DEBOUNCE_MS)
    {
        snapshotActiveNotes(notes);
        notesDirty = false;
        for (int n = 0; n < 128; n++) {
            if (!notes[n]) {
                continue;
            }

            if (!firstNote) {
                heldNotes += " ";
            }

            heldNotes += noteName(n);
            firstNote = false;
        }

        if (heldNotes.length() == 0) {
            heldNotes = "- - -";
        }
        
        /*
        -- void drawScreen explanation --
        const char* topHeader, -- Red zone: Flats or sharps of a key.
        const char* chordDisplay, -- Green zone: The root note display, e.g. "C", "D#", "F", etc.
        const char* chordQuality, -- Magenta zone: The chord quality display, e.g. "maj7", "m", "dim", etc.
        const char* bottomHeader, -- Cyan/Yellow zone: The note list display, e.g. "C4 E4 G4" for a C major chord
        const char* chordAlternatives[], -- Blue zones: The alternative names of the chord, like chords that share the same pair of notes
        int numAlternatives -- Number of chord alternatives (0 minimum, 6 maximum)
        */

        drawScreen(
            "-",
            "--",
            "--",
            heldNotes.c_str(),
            nullptr,
            0
        );
    }
}