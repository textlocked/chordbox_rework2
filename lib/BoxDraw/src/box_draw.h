#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

enum Alignment {
    LEFT,
    CENTER,
    RIGHT
};

struct Point {
    int16_t x;
    int16_t y;
};

extern TFT_eSPI tft;
extern int prevTextSize;

uint16_t createRGB565(uint8_t r, uint8_t g, uint8_t b);

void tftPrintInBox(const char* text, Point topleft, Point topright,
                  Point bottomleft, Point bottomright, uint16_t color,
                  bool clear, bool bold, int size, Alignment align);

void box_TopHeader(const char* text, uint16_t color);
void box_ChordDisplay(const char* text, uint16_t color);
void box_ChordQuality(const char* text, uint16_t color);
void box_BottomHeader(const char* text, uint16_t color);
void box_ChordAlternatives(const char* text, uint16_t color, int index);
void drawScreen(const char* topHeader, const char* chordDisplay,
               const char* chordQuality, const char* bottomHeader,
               const char* chordAlternatives[], int numAlternatives,
               int preferredCandidateIndex = -1,
               const char* topLeftHeader = "", const char* topRightHeader = "");
