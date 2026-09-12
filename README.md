# ChordBox Rework 2

ChordBox is a standalone MIDI chord display built around a Raspberry Pi Pico/RP2040. It:

- Hosts a USB MIDI keyboard through the Pico's native USB-C port.
- Tracks all currently held MIDI notes, including notes held by the sustain pedal.
- Displays the held notes and detected chord on an ST7789 TFT.
- Shows alternative chord names when the detector finds multiple candidates.
- Lets the encoder select and save a preferred chord interpretation for a note set.
- Toggles the TFT orientation between rotations 3 and 1 when the KY040 button is held.
- Keeps USB MIDI servicing on one RP2040 core and the display/UI work on the other.

## Why I Made This

I am a music producer that just happens to have robotics as their hobby. Or the other way around, really. Both weigh the same, but I am more into arts... and maths, well, still the same weight.

I created this project because I wanted a simple, fast way to look up what the chord I just played on my keyboard was. I sometimes improvise something and hit a very beautiful chord, and I could not really spend my time searching Google to find out what that chord was. So, why not make it an instant screen that I could put right on my keyboard?

I also created this because MIDI chord detection is usually software: something downloadable to a device, usually a desktop computer. I wanted this to be a portable chord lookupper. Useful when doing gigs, eh?

Anyway, enough storytelling. Here is how to set it up on an RP2040 microcontroller.

## Hardware

The current firmware is configured for:

- Raspberry Pi Pico or compatible RP2040 board
- ST7789 170x320 TFT display
- KY040 rotary encoder
- USB MIDI keyboard or other USB MIDI device
- USB host wiring/power suitable for the MIDI device

### Pinout

| Device | Signal | Pico GPIO |
| --- | --- | ---: |
| ST7789 | CS | 17 |
| ST7789 | DC | 20 |
| ST7789 | RST | 21 |
| ST7789 | SDA | 19 |
| ST7789 | SCK | 18 |
| ST7789 | Backlight | 22 |
| KY040 | CLK | 6 |
| KY040 | DT | 7 |
| KY040 | SW | 8 |
| Serial debug | TX | 0 |
| Serial debug | RX | 1 |

The display uses hardware SPI0. Connect grounds together. Make sure the display and MIDI device receive appropriate power, and check the voltage requirements of the particular TFT and encoder module before wiring them.

The Pico's native USB-C connector is used as a USB host for the MIDI keyboard. A suitable USB OTG adapter, hub, or host cable may be required depending on the keyboard and Pico hardware arrangement.

## Controls

### KY040 rotation

- Turn the encoder to move through chord candidates.
- Press and release the encoder button to toggle sustain-control mode.
- Hold the encoder button for about 700 ms to toggle the TFT rotation:
  - Default: rotation 3, landscape layout
  - Next hold: rotation 1
  - Next hold: rotation 3 again

A hold is treated separately from a short press, so holding the button does not toggle sustain or reset a chord preference.

### Chord preferences

When more than one chord interpretation is available, turn the encoder to choose a candidate. The selected candidate is saved in EEPROM using the note mask, so the preference can be restored when that note combination is encountered again.

## Software Setup

This is a PlatformIO project. Install:

- Visual Studio Code
- The PlatformIO extension
- Git, so PlatformIO can retrieve the configured Arduino-Pico core and Raspberry Pi platform

Open the `chordbox_rework2` folder in VS Code, then build it with PlatformIO.

From a terminal, the equivalent commands are:

```text
platformio run
platformio run --target upload
```

The project is configured to upload with `picotool`:

```ini
upload_protocol = picotool
```

If PlatformIO cannot find the command in your terminal, use the executable inside the PlatformIO virtual environment, for example:

```text
C:\Users\<your-user>\.platformio\penv\Scripts\platformio.exe run
```

The upload port may need to be selected manually if more than one board or serial device is connected.

## Important PlatformIO Configuration

The project uses the Earle Philhower Arduino-Pico core rather than the Arduino Mbed RP2040 core. This is important because the TinyUSB RP2040 code depends on the PIO API provided by the selected core.

The `platformio.ini` file enables:

- Arduino-Pico for the Raspberry Pi Pico
- TinyUSB host support
- TinyUSB MIDI host support
- A larger enumeration buffer
- TFT_eSPI with ST7789 and the current display pin configuration
- TFT_eSPI as the display library
- The local chord detector and BoxDraw libraries from `lib/`

Do not switch the project back to the Mbed RP2040 framework without also checking the TinyUSB, PIO, and display-library compatibility.

## Project Structure

```text
chordbox_rework2/
|-- include/       Generated font headers used by the TFT UI
|-- lib/
|   |-- BoxDraw/   TFT layout, text rendering, and redraw cache
|   `-- ChordDetector/ Chord matching and candidate generation
|-- src/
|   `-- main.cpp   USB MIDI host, note tracking, controls, and UI loop
|-- platformio.ini PlatformIO board, library, and TinyUSB configuration
`-- README.md      This document
```

## Firmware Architecture

The RP2040 core split is intentional:

- `setup()` and `loop()` keep the native USB host running as quickly as possible.
- `setup1()` and `loop1()` handle the TFT, encoder, EEPROM, chord detection, and display updates.
- MIDI callbacks update the active-note state and mark the display as dirty.
- The display waits briefly for a quiet gap after MIDI activity before redrawing, which helps a fast chord arrive as one group instead of several partial screen updates.

The firmware uses TinyUSB's native MIDI host callbacks and the raw four-byte USB-MIDI packet format. It does not use `usb_midi_host`, `EZ_USB_MIDI_HOST`, or the Arduino MIDI Library.

## Troubleshooting

### The display is blank

- Check the TFT power and shared ground.
- Confirm the ST7789 wiring matches the pinout above.
- Confirm the backlight pin is wired to either GPIO 22, a 3.3V source, or update the corresponding constant in the firmware.
- Check that the display dimensions and SPI settings in `platformio.ini` match the panel!!!! (should be seen by prefix "-DTFT")

### The keyboard is not detected

- Confirm the keyboard is connected to the Pico's native USB port. I used an OTG adapter to connect USB-A to my Pico's USB-C port.
- Check host power. Some keyboards need more current than a Pico can provide directly.
- Watch the Serial1 debug output for `MIDI MOUNT`. 

### The encoder moves the wrong way

Swap the KY040 `CLK` and `DT` wires, or reverse the direction logic in `pollEncoder()`.

### Chord candidates are missing

The current build enables one missing-note alternative and no extra-note alternatives through the PlatformIO build flags. The chord detector is intentionally conservative to avoid presenting implausible matches.

## Current Status

This is an in-progress hardware project. The main path is working toward a portable, dedicated chord lookup display for live playing and improvisation. Expect to adjust pin assignments, display dimensions, fonts, and mechanical mounting for the specific hardware build.
