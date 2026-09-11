#include "box_draw.h"

#include "../../../include/Helvetica7pt7b.h"
#include "../../../include/Helvetica9pt7b.h"
#include "../../../include/Helvetica12pt7b.h"
#include "../../../include/Helvetica18pt7b.h"
#include "../../../include/Helvetica20pt7b.h"
#include "../../../include/Helvetica24pt7b.h"
#include "../../../include/Helvetica44pt7b.h"
#include "../../../include/Helvetica_Bold7pt7b.h"
#include "../../../include/Helvetica_Bold9pt7b.h"
#include "../../../include/Helvetica_Bold12pt7b.h"
#include "../../../include/Helvetica_Bold18pt7b.h"
#include "../../../include/Helvetica_Bold20pt7b.h"
#include "../../../include/Helvetica_Bold24pt7b.h"
#include "../../../include/Helvetica_Bold44pt7b.h"

TFT_eSPI tft = TFT_eSPI();
int prevTextSize = 0;

uint16_t createRGB565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

void tftPrintInBox(const char* text, Point topleft, Point topright, Point bottomleft, Point bottomright, uint16_t color, bool clear, bool bold, int size, Alignment align) {
    const GFXfont* selectedFont = nullptr;

    if (clear) {
        tft.fillScreen(TFT_BLACK);
    } else {
        const int16_t left = min(topleft.x, bottomleft.x);
        const int16_t right = max(topright.x, bottomright.x);
        const int16_t top = min(topleft.y, topright.y);
        const int16_t bottom = max(bottomleft.y, bottomright.y);
        tft.fillRect(left, top, right - left, bottom - top, TFT_BLACK);
    }

    if (bold) {
        if (size == 7) {
            tft.setFreeFont(&Helvetica_Bold7pt7b);
            selectedFont = &Helvetica_Bold7pt7b;
        } else if (size == 9) {
            tft.setFreeFont(&Helvetica_Bold9pt7b);
            selectedFont = &Helvetica_Bold9pt7b;
        } else if (size == 12) {
            tft.setFreeFont(&Helvetica_Bold12pt7b);
            selectedFont = &Helvetica_Bold12pt7b;
        } else if (size == 18) {
            tft.setFreeFont(&Helvetica_Bold18pt7b);
            selectedFont = &Helvetica_Bold18pt7b;
        } else if (size == 20) {
            tft.setFreeFont(&Helvetica_Bold20pt7b);
            selectedFont = &Helvetica_Bold20pt7b;
        } else if (size == 24) {
            tft.setFreeFont(&Helvetica_Bold24pt7b);
            selectedFont = &Helvetica_Bold24pt7b;
        } else if (size == 44) {
            tft.setFreeFont(&Helvetica_Bold44pt7b);
            selectedFont = &Helvetica_Bold44pt7b;
        }
    } else {
        if (size == 7) {
            tft.setFreeFont(&Helvetica7pt7b);
            selectedFont = &Helvetica7pt7b;
        } else if (size == 9) {
            tft.setFreeFont(&Helvetica9pt7b);
            selectedFont = &Helvetica9pt7b;
        } else if (size == 12) {
            tft.setFreeFont(&Helvetica12pt7b);
            selectedFont = &Helvetica12pt7b;
        } else if (size == 18) {
            tft.setFreeFont(&Helvetica18pt7b);
            selectedFont = &Helvetica18pt7b;
        } else if (size == 20) {
            tft.setFreeFont(&Helvetica20pt7b);
            selectedFont = &Helvetica20pt7b;
        } else if (size == 24) {
            tft.setFreeFont(&Helvetica24pt7b);
            selectedFont = &Helvetica24pt7b;
        } else if (size == 44) {
            tft.setFreeFont(&Helvetica44pt7b);
            selectedFont = &Helvetica44pt7b;
        }
    }

    if (selectedFont == nullptr) {
        tft.setFreeFont(&Helvetica12pt7b);
        selectedFont = &Helvetica12pt7b;
    }

    int16_t glyphTop = 0;
    int16_t glyphBottom = 0;
    bool hasGlyph = false;

    for (const char* character = text; *character != '\0'; ++character) {
        const uint16_t code = static_cast<uint8_t>(*character);
        const uint16_t first = pgm_read_word(&selectedFont->first);
        const uint16_t last = pgm_read_word(&selectedFont->last);
        if (code < first || code > last) {
            continue;
        }

        GFXglyph* glyphs = reinterpret_cast<GFXglyph*>(pgm_read_dword(&selectedFont->glyph));
        const GFXglyph* glyph = &glyphs[code - first];
        const int16_t topOffset = static_cast<int8_t>(pgm_read_byte(&glyph->yOffset));
        const int16_t bottomOffset = topOffset + pgm_read_byte(&glyph->height);

        if (!hasGlyph || topOffset < glyphTop) {
            glyphTop = topOffset;
        }
        if (!hasGlyph || bottomOffset > glyphBottom) {
            glyphBottom = bottomOffset;
        }
        hasGlyph = true;
    }

    const int16_t left = min(topleft.x, bottomleft.x);
    const int16_t right = max(topright.x, bottomright.x);
    const int16_t top = min(topleft.y, topright.y);
    const int16_t bottom = max(bottomleft.y, bottomright.y);
    const int16_t boxWidth = right - left;
    const int16_t boxHeight = bottom - top;

    tft.setTextColor(color, TFT_BLACK);

    const int16_t lineHeight = pgm_read_byte(&selectedFont->yAdvance);
    int16_t lineCount = 1;
    for (const char* character = text; *character != '\0'; ++character) {
        if (*character == '\n') {
            ++lineCount;
        }
    }

    const int16_t visibleHeight = glyphBottom - glyphTop;
    const int16_t totalTextHeight = visibleHeight + (lineCount - 1) * lineHeight;
    const int16_t firstCursorY = top + (boxHeight - totalTextHeight) / 2 - glyphTop;

    const char* lineStart = text;
    int16_t lineNumber = 0;
    while (lineStart != nullptr) {
        const char* lineEnd = strchr(lineStart, '\n');
        String line = lineEnd == nullptr
            ? String(lineStart)
            : String(lineStart).substring(0, lineEnd - lineStart);
        const int16_t lineWidth = tft.textWidth(line.c_str());
        int16_t cursorX;

        if (align == CENTER) {
            cursorX = left + (boxWidth - lineWidth) / 2;
        } else if (align == RIGHT) {
            cursorX = right - lineWidth;
        } else {
            cursorX = left;
        }

        tft.setCursor(cursorX, firstCursorY + lineNumber * lineHeight);
        tft.print(line);
        ++lineNumber;

        if (lineEnd == nullptr) {
            break;
        }
        lineStart = lineEnd + 1;
    }

    prevTextSize = size;
}

void box_TopHeader(const char* text, uint16_t color) {
    static String previousText;
    if (previousText == text) {
        return;
    }
    previousText = text;
    tftPrintInBox(text, {0, 0}, {320, 0}, {0, 16}, {320, 16}, color, false, false, 7, CENTER);
}

void box_ChordDisplay(const char* text, uint16_t color) {
    static String previousText;
    if (previousText == text) {
        return;
    }
    previousText = text;
    tftPrintInBox(text, {7, 19}, {160, 19}, {7, 107}, {160, 107}, color, false, true, 44, LEFT);
}

void box_ChordQuality(const char* text, uint16_t color) {
    static String previousText;
    if (previousText == text) {
        return;
    }
    previousText = text;
    tftPrintInBox(text, {7, 107}, {160, 107}, {7, 151}, {160, 151}, color, false, false, 12, LEFT);
}

void box_BottomHeader(const char* text, uint16_t color) {
    static String previousText;
    if (previousText == text) {
        return;
    }
    previousText = text;
    tftPrintInBox(text, {0, 154}, {320, 154}, {0, 170}, {320, 170}, color, false, false, 7, CENTER);
}

void box_ChordAlternatives(const char* text, uint16_t color, int index) {
    if (index < 1 || index > 6) {
        return;
    }

    static String previousText[6];
    if (previousText[index - 1] == text) {
        return;
    }
    previousText[index - 1] = text;

    if (index == 1) {
        tftPrintInBox(text, {160, 19}, {320, 19}, {160, 41}, {320, 41}, color, false, false, 7, LEFT);
    } else if (index == 2) {
        tftPrintInBox(text, {160, 41}, {320, 41}, {160, 63}, {320, 63}, color, false, false, 7, LEFT);
    } else if (index == 3) {
        tftPrintInBox(text, {160, 63}, {320, 63}, {160, 85}, {320, 85}, color, false, false, 7, LEFT);
    } else if (index == 4) {
        tftPrintInBox(text, {160, 85}, {320, 85}, {160, 107}, {320, 107}, color, false, false, 7, LEFT);
    } else if (index == 5) {
        tftPrintInBox(text, {160, 107}, {320, 107}, {160, 129}, {320, 129}, color, false, false, 7, LEFT);
    } else if (index == 6) {
        tftPrintInBox(text, {160, 129}, {320, 129}, {160, 151}, {320, 151}, color, false, false, 7, LEFT);
    }
}

void drawScreen(const char* topHeader, const char* chordDisplay, const char* chordQuality, const char* bottomHeader, const char* chordAlternatives[], int numAlternatives) {
    box_TopHeader(topHeader, TFT_WHITE);
    box_ChordDisplay(chordDisplay, TFT_WHITE);
    box_ChordQuality(chordQuality, createRGB565(255, 255, 0));
    box_BottomHeader(bottomHeader, TFT_WHITE);

    for (int i = 0; i < 6; ++i) {
        const char* alternative = i < numAlternatives ? chordAlternatives[i] : "";
        box_ChordAlternatives(alternative, createRGB565(200, 200, 200), i + 1);
    }
}
