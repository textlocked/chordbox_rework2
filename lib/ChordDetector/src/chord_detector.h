// chord_detector.h
#pragma once
#include <stdint.h>

struct ChordResult {
    uint8_t rootPc;
    uint8_t bassPc;
    const char* fullName;   // e.g. "minor 7 b5"
    const char* shorthand;  // e.g. "\xC3\xB8" (ø)
    bool isMajorFamily;
    bool isDiminished;
    bool found;
};

uint8_t detectChordCandidates(uint16_t noteMask, uint8_t lowestPc,
                              ChordResult* results, uint8_t maxResults);
ChordResult detectChord(uint16_t noteMask, uint8_t lowestPc);