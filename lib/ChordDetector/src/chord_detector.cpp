// chord_detector.cpp
#include "chord_detector.h"

// Readable interval-to-bitmask helpers
#define M1(a) (1u<<(a))
#define M2(a,b) (M1(a)|M1(b))
#define M3(a,b,c) (M1(a)|M1(b)|M1(c))
#define M4(a,b,c,d) (M1(a)|M1(b)|M1(c)|M1(d))
#define M5(a,b,c,d,e) (M1(a)|M1(b)|M1(c)|M1(d)|M1(e))
#define M6(a,b,c,d,e,f) (M1(a)|M1(b)|M1(c)|M1(d)|M1(e)|M1(f))
#define M7(a,b,c,d,e,f,g) (M1(a)|M1(b)|M1(c)|M1(d)|M1(e)|M1(f)|M1(g))

struct ChordTemplate {
    uint16_t mask;
    const char* fullName;
    const char* shorthand;
    bool isMajorFamily;
    bool isDiminished;
};

static const ChordTemplate TEMPLATES[] = {
    // triads
    {M3(0,4,7),        "major",              "",        true,  false},
    {M3(0,3,7),         "minor",              "m",       false, false},
    {M3(0,3,6),         "diminished",         "\xC2\xB0",false, true },
    {M3(0,4,8),         "augmented",          "+",       true,  false},
    {M3(0,2,7),         "sus2",               "sus2",    true,  false},
    {M3(0,5,7),         "sus4",               "sus4",    true,  false},
    {M4(0,2,5,7),       "sus2sus4",           "sus24",   true,  false},

    // sixths
    {M4(0,4,7,9),       "major 6",            "6",       true,  false},
    {M4(0,3,7,9),       "minor 6",            "m6",      false, false},
    {M4(0,4,7,8),       "major add b6",       "addb6",   true,  false},
    {M4(0,3,7,8),       "minor add b6",       "mb6",     false, false},

    // sevenths
    {M4(0,4,7,11),      "major 7",            "maj7",    true,  false},
    {M4(0,3,7,10),      "minor 7",            "m7",      false, false},
    {M4(0,3,6,9),       "diminished 7",       "\xC2\xB0" "7",false,true },
    {M4(0,3,6,10),      "minor 7 b5",         "\xC3\xB8",false, true },
    {M4(0,4,8,10),      "augmented 7",        "+7",      true,  false},
    {M4(0,4,7,10),      "dominant 7",         "7",       true,  false},
    {M4(0,4,8,11),      "augmented major 7",  "maj7#5",  true,  false},
    {M4(0,3,7,11),      "minor major 7",      "mMaj7",   false, false},

    // adds
    {M4(0,2,4,7),       "major add 9",        "add9",    true,  false},
    {M4(0,2,3,7),       "minor add 9",        "madd9",   false, false},
    {M4(0,1,4,7),       "major add b9",       "addb9",   true,  false},
    {M4(0,1,3,7),       "minor add b9",       "maddb9",  false, false},

    // sixth/ninth
    {M5(0,2,4,7,9),     "major 6/9",          "6/9",     true,  false},
    {M5(0,2,3,7,9),     "minor 6/9",          "m6/9",    false, false},

    // extended
    {M5(0,2,4,7,11),    "major 9",            "maj9",    true,  false},
    {M6(0,2,4,5,7,11),  "major 11",           "maj11",   true,  false},
    {M6(0,2,4,7,9,11),  "major 13",           "maj13",   true,  false},
    {M5(0,2,3,7,10),    "minor 9",            "m9",      false, false},
    {M6(0,2,3,5,7,10),  "minor 11",           "m11",     false, false},
    {M7(0,2,3,5,7,9,10),"minor 13",           "m13",     false, false},
    {M5(0,2,4,7,10),    "dominant 9",         "9",       true,  false},
    {M6(0,2,4,5,7,10),  "dominant 11",        "11",      true,  false},
    {M7(0,2,4,5,7,9,10),"dominant 13",        "13",      true,  false},

    // altered dominants (interpreted as 7th-chord context, see note above)
    {M5(0,1,4,7,10),    "dominant 7 b9",      "7b9",     true,  false},
    {M5(0,3,4,7,10),    "dominant 7 #9",      "7#9",     true,  false},
    {M6(0,2,4,6,7,10),  "dominant 9 #11",     "9#11",    true,  false},
    {M6(0,2,4,7,8,10),  "dominant 9 b13",     "9b13",    true,  false},

    // sus dominants
    {M4(0,5,7,10),      "dominant 7 sus4",    "7sus4",   true,  false},
    {M4(0,2,7,10),      "dominant 7 sus2",    "7sus2",   true,  false},

    // dominant adds
    {M5(0,4,5,7,10),    "dominant 7 add 11",  "7(add11)",true,  false},
    {M5(0,4,7,9,10),    "dominant 7 add 13",  "7(add13)",true,  false},
};
static const uint8_t NUM_TEMPLATES = sizeof(TEMPLATES) / sizeof(TEMPLATES[0]);

static uint16_t rotateRight12(uint16_t mask, uint8_t root) {
    return ((mask >> root) | (mask << (12 - root))) & 0x0FFF;
}

static uint8_t popcount12(uint16_t v) {
    uint8_t c = 0;
    for (uint8_t i = 0; i < 12; i++) if (v & (1 << i)) c++;
    return c;
}

ChordResult detectChord(uint16_t noteMask, uint8_t lowestPc) {
    ChordResult best = {0, lowestPc, "unknown", "?", true, false, false};
    int bestScore = -100;

    for (uint8_t root = 0; root < 12; root++) {
        if (!(noteMask & (1 << root))) continue;
        uint16_t rotated = rotateRight12(noteMask, root);

        for (uint8_t t = 0; t < NUM_TEMPLATES; t++) {
            uint16_t tmpl = TEMPLATES[t].mask;
            uint8_t matched = popcount12(rotated & tmpl);
            uint8_t missing = popcount12(tmpl & ~rotated);
            uint8_t extra   = popcount12(rotated & ~tmpl);
            int score = matched * 3 - missing * 2 - extra * 2;

            if (score > bestScore) {
                bestScore = score;
                best.rootPc = root;
                best.bassPc = lowestPc;
                best.fullName = TEMPLATES[t].fullName;
                best.shorthand = TEMPLATES[t].shorthand;
                best.isMajorFamily = TEMPLATES[t].isMajorFamily;
                best.isDiminished = TEMPLATES[t].isDiminished;
                best.found = true;
            }
        }
    }
    return best;
}