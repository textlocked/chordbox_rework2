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

    // ============================================================
    // TRIADS
    // ============================================================

    {M3(0,4,7),        "major",              "",          true,  false},
    {M3(0,3,7),        "minor",              "m",         false, false},
    {M3(0,3,6),        "dim",                "dim",       false, true },
    {M3(0,4,8),        "aug",                "+",         true,  false},
    {M3(0,2,7),        "sus2",               "sus2",      true,  false},
    {M3(0,5,7),        "sus4",               "sus4",      true,  false},
    {M4(0,2,5,7),      "sus2sus4",           "sus2/4",    true,  false},

    // ============================================================
    // SIXTHS
    // ============================================================

    {M4(0,4,7,9),       "maj(6)",             "6",         true,  false},
    {M4(0,3,7,9),       "min(6)",             "m6",        false, false},
    {M4(0,4,7,8),       "maj(b6)",            "maj b6",    true,  false},
    {M4(0,3,7,8),       "min(b6)",            "m b6",      false, false},

    // ============================================================
    // SEVENTHS
    // ============================================================

    {M4(0,4,7,11),      "maj7",               "maj7",      true,  false},
    {M4(0,3,7,10),      "min7",               "m7",        false, false},
    {M4(0,3,6,9),       "dim7",               "dim7",      false, true },
    {M4(0,3,6,10),      "min7(b5)",           "hdim",      false, true },
    {M4(0,4,8,10),      "aug7",               "+7",        true,  false},
    {M4(0,4,7,10),      "7",                  "7",         true,  false},
    {M4(0,4,8,11),      "augMaj7",            "maj7#5",    true,  false},
    {M4(0,3,7,11),      "minMaj7",             "mMaj7",     false, false},

    // ============================================================
    // ADD CHORDS
    // ============================================================

    {M4(0,2,4,7),       "maj(add 9)",         "add9",      true,  false},
    {M4(0,2,3,7),       "min(add 9)",         "madd9",     false, false},
    {M4(0,1,4,7),       "maj(add b9)",        "addb9",     true,  false},
    {M4(0,1,3,7),       "min(add b9)",        "maddb9",    false, false},

    {M4(0,4,5,7),       "maj(add 11)",       "add11",     true,  false},
    {M4(0,4,7,9),       "maj(add 13)",       "add13",     true,  false},
    {M4(0,4,6,7),       "maj(add #11)",      "add#11",    true,  false},

    {M4(0,3,5,7),       "min(add 11)",       "madd11",    false, false},
    {M4(0,3,7,9),       "min(add 13)",       "madd13",    false, false},
    {M4(0,3,6,7),       "min(add #11)",      "madd#11",   false, false},

    // ============================================================
    // SIXTH / NINTH
    // ============================================================

    {M5(0,2,4,7,9),     "maj 6/9",           "6/9",       true,  false},
    {M5(0,2,3,7,9),     "min 6/9",           "m6/9",      false, false},

    {M5(0,2,4,6,7),     "6/9(#11)",          "6/9#11",    true,  false},
    {M5(0,2,4,7,8),     "6/9(b13)",          "6/9b13",    true,  false},

    {M5(0,2,3,6,7),     "min 6/9(#11)",      "m6/9#11",   false, false},
    {M5(0,2,3,7,8),     "min b6/9",       "m b6/9",    false, false},

    // ============================================================
    // MAJOR 7 / MINOR-MAJOR 7
    // ============================================================

    {M5(0,2,4,7,11),     "maj9",              "maj9",      true,  false},
    {M6(0,2,4,5,7,11),   "maj11",             "maj11",     true,  false},
    {M6(0,2,4,7,9,11),   "maj13",             "maj13",     true,  false},

    {M6(0,2,4,6,7,11),   "maj9(#11)",         "maj9#11",   true,  false},
    {M7(0,2,4,6,7,9,11), "maj13(#11)",        "maj13#11",  true,  false},

    {M5(0,2,4,7,11),     "maj7(add9)",        "maj7add9",  true,  false},
    {M5(0,4,5,7,11),     "maj7(add11)",       "maj7add11", true,  false},
    {M5(0,4,7,9,11),     "maj7(add13)",       "maj7add13", true,  false},
    {M5(0,1,4,7,11),     "maj7(b9)",          "maj7b9",    true,  false},
    {M5(0,4,7,8,11),     "maj7(b13)",         "maj7b13",   true,  false},
    {M5(0,4,6,7,11),     "maj7(#11)",         "maj7#11",   true,  false},
    {M4(0,4,6,11),       "maj7(b5)",          "maj7b5",    true,  false},

    {M5(0,2,3,7,11),     "minMaj9",           "mMaj9",     false, false},
    {M6(0,2,3,5,7,11),   "minMaj11",          "mMaj11",    false, false},
    {M7(0,2,3,5,7,9,11), "minMaj13",          "mMaj13",    false, false},

    {M5(0,3,7,8,11),     "minMaj7(b6)",       "mMaj7b6",   false, false},
    {M5(0,3,6,7,11),     "minMaj7(b5)",       "mMaj7b5",   false, false},

    // ============================================================
    // MINOR EXTENSIONS
    // ============================================================

    {M5(0,2,3,7,10),     "min9",              "m9",        false, false},
    {M6(0,2,3,5,7,10),   "min11",             "m11",       false, false},
    {M7(0,2,3,5,7,9,10), "min13",             "m13",       false, false},

    {M6(0,2,3,6,7,10),   "min9(#11)",         "m9#11",     false, false},
    {M7(0,2,3,6,7,9,10), "min13(#11)",        "m13#11",    false, false},
    {M6(0,2,3,7,8,10),   "min9(b13)",         "m9b13",     false, false},
    {M7(0,2,3,5,7,8,10), "min13(b13)",        "m13b13",    false, false},

    // ============================================================
    // DOMINANT 7 ALTERATIONS
    // ============================================================

    {M5(0,1,4,7,10),      "7(b9)",             "7b9",       true,  false},
    {M5(0,3,4,7,10),      "7(#9)",             "7#9",       true,  false},
    {M6(0,2,4,6,7,10),    "9(#11)",            "9#11",      true,  false},
    {M6(0,2,4,7,8,10),    "9(b13)",            "9b13",      true,  false},

    {M4(0,4,6,10),        "7(b5)",             "7b5",       true,  false},
    {M4(0,4,8,10),        "7(#5)",             "7#5",       true,  false},
    {M5(0,4,6,7,10),      "7(#11)",            "7#11",      true,  false},
    {M5(0,4,7,8,10),      "7(b13)",            "7b13",      true,  false},

    {M6(0,1,4,6,7,10),    "7(b9,#11)",        "7b9#11",    true,  false},
    {M6(0,3,4,6,7,10),    "7(#9,#11)",        "7#9#11",    true,  false},
    {M6(0,1,4,7,8,10),    "7(b9,b13)",        "7b9b13",    true,  false},
    {M6(0,3,4,7,8,10),    "7(#9,b13)",        "7#9b13",    true,  false},
    {M6(0,4,6,8,10,7),    "7(#11,b13)",       "7#11b13",   true,  false},

    {M5(0,1,4,6,10),      "7(b5,b9)",         "7b5b9",     true,  false},
    {M5(0,3,4,6,10),      "7(b5,#9)",         "7b5#9",     true,  false},
    {M5(0,1,4,8,10),      "7(#5,b9)",         "7#5b9",     true,  false},
    {M5(0,3,4,8,10),      "7(#5,#9)",         "7#5#9",     true,  false},

    {M6(0,1,4,7,8,10),    "9(b9,b13)",        "9b9b13",    true,  false},
    {M6(0,3,4,7,8,10),    "9(#9,b13)",        "9#9b13",    true,  false},
    {M6(0,1,4,6,7,10),    "9(b9,#11)",        "9b9#11",    true,  false},
    {M6(0,3,4,6,7,10),    "9(#9,#11)",        "9#9#11",    true,  false},
    {M6(0,2,4,6,8,10),    "9(#11,b13)",       "9#11b13",   true,  false},

    // ============================================================
    // EXTENDED DOMINANTS
    // ============================================================

    {M6(0,2,4,5,7,10),    "11",                "11",        true,  false},
    {M7(0,2,4,5,7,9,10),  "13",                "13",        true,  false},

    {M7(0,2,4,6,7,9,10),  "13(#11)",           "13#11",    true,  false},
    {M7(0,2,4,5,7,8,10),  "13(b13)",           "13b13",    true,  false},

    // ============================================================
    // SUSPENDED DOMINANTS
    // ============================================================

    {M4(0,5,7,10),         "7sus4",             "7sus4",     true,  false},
    {M4(0,2,7,10),         "7sus2",             "7sus2",     true,  false},

    {M5(0,2,5,7,10),       "9sus4",             "9sus4",     true,  false},
    {M5(0,2,7,9,10),       "9sus2",             "9sus2",     true,  false},
    {M6(0,2,5,7,9,10),     "13sus4",            "13sus4",    true,  false},
    {M6(0,2,7,9,10,5),     "13sus2",            "13sus2",    true,  false},

    {M6(0,2,5,7,6,10),     "9sus4(#11)",        "9sus4#11",  true,  false},
    {M7(0,2,5,7,9,10,6),   "13sus4(#11)",       "13sus4#11", true,  false},

    // ============================================================
    // DOMINANT ADDS
    // ============================================================

    {M5(0,4,5,7,10),       "7(add11)",          "7add11",    true,  false},
    {M5(0,4,7,9,10),       "7(add13)",          "7add13",    true,  false},

    // ============================================================
    // AUGMENTED EXTENSIONS
    // ============================================================

    {M5(0,2,4,8,10),       "aug9",              "+9",        true,  false},
    {M5(0,2,4,8,11),       "augMaj9",           "+maj9",     true,  false},
    {M6(0,2,4,5,8,11),     "augMaj11",          "+maj11",    true,  false},
    {M7(0,2,4,5,8,9,11),   "augMaj13",          "+maj13",    true,  false},
    {M6(0,2,4,8,9,10),     "aug13",             "+13",       true,  false},
    {M6(0,4,6,8,10,11),    "augMaj7(#11)",      "+maj7#11",  true,  false},

    // ============================================================
    // DIMINISHED / HALF-DIMINISHED
    // ============================================================

    {M4(0,4,6,11),         "maj7(b5)",          "maj7b5",    true,  false}, // from original list

    {M4(0,3,6,11),         "dimMaj7",           "dimMaj7",   false, true },
    {M5(0,2,3,6,9),        "dim9",              "dim9",      false, true },
    {M4(0,1,3,6),          "dim(b9)",            "dimb9",     false, true },
    {M6(0,2,3,5,6,9),      "dim11",             "dim11",     false, true },
    {M7(0,2,3,5,6,8,9),    "dim13",             "dim13",     false, true },

    {M5(0,2,3,6,10),       "hdim9",             "ø9",        false, true },
    {M6(0,2,3,5,6,10),     "hdim11",            "ø11",       false, true },
    {M7(0,2,3,5,6,8,10),   "hdim13",            "ø13",       false, true },

    {M5(0,2,3,6,11),       "dimMaj9",           "dimMaj9",   false, true },
    {M6(0,2,3,5,6,11),     "dimMaj11",          "dimMaj11",  false, true },
    {M7(0,2,3,5,6,9,11),   "dimMaj13",          "dimMaj13",  false, true },

    // ============================================================
    // "ALT" STYLE DOMINANT
    // ============================================================

    {M6(0,1,3,4,7,10),     "7(b9,#9)",         "7b9#9",     true,  false},
    {M7(0,1,3,4,6,7,10),   "7alt",             "alt",       true,  false},
};
/*static const ChordTemplate TEMPLATES[] = {
    // triads
    {M3(0,4,7),        "major",              "",        true,  false},
    {M3(0,3,7),         "minor",              "m",       false, false},
    {M3(0,3,6),         "dim",         "dim",     false, true },
    {M3(0,4,8),         "aug",          "+",       true,  false},
    {M3(0,2,7),         "sus2",               "sus2",    true,  false},
    {M3(0,5,7),         "sus4",               "sus4",    true,  false},
    {M4(0,2,5,7),       "sus2sus4",           "sus2/4",   true,  false},

    // sixths
    {M4(0,4,7,9),       "maj(6)",            "6",       true,  false},
    {M4(0,3,7,9),       "min(6)",            "m6",      false, false},
    {M4(0,4,7,8),       "maj(b6)",       "maj b6",   true,  false},
    {M4(0,3,7,8),       "min(b6)",       "m b6",     false, false},

    // sevenths
    {M4(0,4,7,11),      "maj7",            "maj7",    true,  false},
    {M4(0,3,7,10),      "min7",            "m7",      false, false},
    {M4(0,3,6,9),       "dim7",       "dim7",    false,true },
    {M4(0,3,6,10),      "min7(b5)",         "hdim",    false, true },
    {M4(0,4,8,10),      "aug7",        "+7",      true,  false},
    {M4(0,4,7,10),      "7",         "7",       true,  false},
    {M4(0,4,8,11),      "augMaj7",  "maj7#5",  true,  false},
    {M4(0,3,7,11),      "minMaj7",      "mMaj7",   false, false},

    // adds
    {M4(0,2,4,7),       "maj(add 9)",        "add9",    true,  false},
    {M4(0,2,3,7),       "min(add 9)",        "madd9",   false, false},
    {M4(0,1,4,7),       "maj(add b9)",       "addb9",   true,  false},
    {M4(0,1,3,7),       "min(add b9)",       "maddb9",  false, false},

    // sixth/ninth
    {M5(0,2,4,7,9),     "maj 6/9",          "6/9",     true,  false},
    {M5(0,2,3,7,9),     "min 6/9",          "m6/9",    false, false},

    // extended
    {M5(0,2,4,7,11),    "maj9",            "maj9",    true,  false},
    {M6(0,2,4,5,7,11),  "maj11",           "maj11",   true,  false},
    {M6(0,2,4,7,9,11),  "maj13",           "maj13",   true,  false},
    {M5(0,2,3,7,10),    "min9",            "m9",      false, false},
    {M6(0,2,3,5,7,10),  "min11",           "m11",     false, false},
    {M7(0,2,3,5,7,9,10),"min13",           "m13",     false, false},
    {M5(0,2,4,7,10),    "9",         "9",       true,  false},
    {M6(0,2,4,5,7,10),  "11",        "11",      true,  false},
    {M7(0,2,4,5,7,9,10),"13",        "13",      true,  false},

    // altered dominants (interpreted as 7th-chord context, see note above)
    {M5(0,1,4,7,10),    "7(b9)",      "7b9",     true,  false},
    {M5(0,3,4,7,10),    "7(#9)",      "7#9",     true,  false},
    {M6(0,2,4,6,7,10),  "9(#11)",     "9#11",    true,  false},
    {M6(0,2,4,7,8,10),  "9(b13)",     "9b13",    true,  false},

    // sus dominants
    {M4(0,5,7,10),      "7sus4",    "7sus4",   true,  false},
    {M4(0,2,7,10),      "7sus2",    "7sus2",   true,  false},

    // dominant adds
    {M5(0,4,5,7,10),    "7(add11)",  "7add11",true,  false},
    {M5(0,4,7,9,10),    "7(add13)",  "7add13",true,  false},

    // discovered // first true means if it's a major family chord, second true means if it's a diminished chord
    // what qualifies as major: major, aug, dom, sus
    // what qualifies as not major: minor, dim, minmaj, hdim
    // what qualifies as diminished: dim, hdim, dim with extensions

    {M5(0,4,6,7,11),    "maj7(#11)",  "maj7#11",  true,  false},
    {M5(0,3,4,7,9),    "6/9(#11)",  "6/9#11",  true,  false},
    {M4(0,4,6,11),    "maj7(b5)",  "maj7b5",  true,  false},
    {M4(0,4,6,10),      "7(b5)",         "7b5",       true,  false},
    {M4(0,1,3,6),      "dim(b9)",         "dimb9",       false,  true},

    // ============================================================
    // additional add-tone chords
    // ============================================================

    {M4(0,4,7,2),        "maj(add9)",        "add9",       true,  false}, // duplicate concept
    {M4(0,4,7,5),        "maj(add11)",       "add11",      true,  false},
    {M4(0,4,7,9),        "maj(add13)",       "add13",      true,  false},
    {M4(0,4,7,1),        "maj(addb9)",       "addb9",      true,  false},
    {M4(0,4,7,8),        "maj(addb6)",       "addb6",      true,  false},
    {M4(0,4,7,6),        "maj(add#11)",      "add#11",     true,  false},

    {M4(0,3,7,5),        "min(add11)",       "madd11",     false, false},
    {M4(0,3,7,9),        "min(add13)",       "madd13",     false, false},
    {M4(0,3,7,6),        "min(add#11)",      "madd#11",    false, false},
    {M4(0,3,7,1),        "min(addb9)",       "maddb9",     false, false},
    {M4(0,3,7,8),        "min(addb6)",       "maddb6",     false, false},

    // ============================================================
    // sixth variations
    // ============================================================

    {M4(0,4,7,10),       "maj6(add7)",       "6add7",       true,  false},
    {M5(0,2,4,7,8),      "6/9(b13)",         "6/9b13",     true,  false},
    {M5(0,2,4,6,7),      "6/9(#11)",         "6/9#11",     true,  false},

    {M5(0,2,3,7,8),      "min6/9(b6)",       "m6/9b6",     false, false},
    {M5(0,2,3,7,6),      "min6/9(#11)",      "m6/9#11",    false, false},
    {M5(0,2,3,7,10),     "min6/9(b7)",       "m6/9b7",     false, false},

    // ============================================================
    // major 7 variations
    // ============================================================

    {M5(0,2,4,7,10),     "maj7(add9)",       "maj7add9",   true,  false},
    {M5(0,4,5,7,11),     "maj7(add11)",      "maj7add11",  true,  false},
    {M5(0,4,7,9,11),     "maj7(add13)",      "maj7add13",  true,  false},
    {M5(0,1,4,7,11),     "maj7(b9)",         "maj7b9",     true,  false},
    {M5(0,4,7,8,11),     "maj7(b13)",        "maj7b13",    true,  false},
    {M5(0,4,6,7,11),     "maj7(#11)",        "maj7#11",    true,  false},

    // ============================================================
    // minor-major variations
    // ============================================================

    {M5(0,2,3,7,11),      "minMaj9",          "mMaj9",      false, false},
    {M6(0,2,3,5,7,11),    "minMaj11",         "mMaj11",     false, false},
    {M7(0,2,3,5,7,9,11),  "minMaj13",         "mMaj13",     false, false},

    {M5(0,3,7,8,11),      "minMaj7(b6)",      "mMaj7b6",    false, false},
    {M5(0,3,6,7,11),      "minMaj7(b5)",      "mMaj7b5",    false, false},

    // ============================================================
    // dominant 7 alterations
    // ============================================================

    {M5(0,4,6,7,10),      "7(#11)",           "7#11",       true,  false},
    {M5(0,4,7,8,10),      "7(b13)",           "7b13",       true,  false},
    {M5(0,4,6,8,10),      "7(#11,b13)",       "7#11b13",    true,  false},

    {M6(0,1,4,6,7,10),    "7(b9,#11)",        "7b9#11",     true,  false},
    {M6(0,3,4,6,7,10),    "7(#9,#11)",        "7#9#11",     true,  false},
    {M6(0,1,4,7,8,10),    "7(b9,b13)",        "7b9b13",     true,  false},
    {M6(0,3,4,7,8,10),    "7(#9,b13)",        "7#9b13",     true,  false},

    {M5(0,4,6,7,10),      "7(b5)",            "7b5",        true,  false},
    {M5(0,4,7,8,10),      "7(#5)",            "7#5",        true,  false},

    {M5(0,1,4,6,10),      "7(b5,b9)",         "7b5b9",      true,  false},
    {M5(0,3,4,6,10),      "7(b5,#9)",         "7b5#9",      true,  false},
    {M5(0,1,4,8,10),      "7(#5,b9)",         "7#5b9",      true,  false},
    {M5(0,3,4,8,10),      "7(#5,#9)",         "7#5#9",      true,  false},

    // particularly "altered dominant" type sets
    {M7(0,1,3,4,6,7,10),  "7alt",             "alt",        true,  false},
    {M6(0,1,3,4,7,10),    "7(b9,#9)",         "7b9#9",      true,  false},
    {M6(0,1,4,6,8,10),    "7(b9,#11,b13)",    "7b9#11b13",  true,  false},
    {M6(0,3,4,6,8,10),    "7(#9,#11,b13)",    "7#9#11b13",  true,  false},

    // ============================================================
    // 9th chords
    // ============================================================

    {M5(0,2,4,7,11),      "maj9",             "maj9",       true,  false},

    {M5(0,2,3,7,11),      "minMaj9",           "mMaj9",      false, false},

    {M6(0,2,4,5,7,11),    "maj11",            "maj11",      true,  false},
    {M7(0,2,4,5,7,9,11),  "maj13",            "maj13",      true,  false},

    {M6(0,2,4,6,7,11),    "maj9(#11)",        "maj9#11",    true,  false},
    {M7(0,2,4,6,7,9,11),  "maj13(#11)",       "maj13#11",   true,  false},

    {M6(0,2,3,6,7,10),    "min9(#11)",        "m9#11",      false, false},
    {M6(0,2,3,5,7,11),    "minMaj11",         "mMaj11",     false, false},
    {M7(0,2,3,5,7,9,11),  "minMaj13",         "mMaj13",     false, false},

    {M6(0,1,4,7,10,11),   "maj9(b9)",         "maj9b9",     true,  false},
    {M6(0,4,7,8,10,11),   "maj9(b13)",        "maj9b13",    true,  false},

    // ============================================================
    // dominant 9 variations
    // ============================================================

    {M6(0,2,4,6,7,10),    "9(#11)",           "9#11",       true,  false},
    {M6(0,2,4,7,8,10),    "9(b13)",           "9b13",       true,  false},

    {M6(0,1,4,6,7,10),    "9(b9,#11)",        "9b9#11",     true,  false},
    {M6(0,3,4,6,7,10),    "9(#9,#11)",        "9#9#11",     true,  false},
    {M6(0,1,4,7,8,10),    "9(b9,b13)",        "9b9b13",     true,  false},
    {M6(0,3,4,7,8,10),    "9(#9,b13)",        "9#9b13",     true,  false},

    {M6(0,2,4,6,8,10),    "9(#11,b13)",       "9#11b13",    true,  false},

    // ============================================================
    // 11th variations
    // ============================================================

    {M6(0,2,4,5,7,10),    "11",               "11",         true,  false},
    {M6(0,2,4,6,7,10),    "11(#11)",          "11#11",      true,  false},

    {M7(0,2,4,5,7,9,10),  "13",               "13",         true,  false},
    {M7(0,2,4,6,7,9,10),  "13(#11)",          "13#11",      true,  false},
    {M7(0,2,4,5,7,8,10),  "13(b13)",          "13b13",      true,  false},

    // ============================================================
    // minor extensions
    // ============================================================

    {M5(0,2,3,7,10),      "min9",             "m9",         false, false},
    {M6(0,2,3,5,7,10),    "min11",            "m11",        false, false},
    {M7(0,2,3,5,7,9,10),  "min13",            "m13",        false, false},

    {M6(0,2,3,6,7,10),    "min9(#11)",        "m9#11",      false, false},
    {M7(0,2,3,6,7,9,10),  "min13(#11)",       "m13#11",     false, false},

    {M6(0,2,3,7,8,10),    "min9(b13)",        "m9b13",      false, false},
    {M7(0,2,3,5,7,8,10),  "min13(b13)",       "m13b13",     false, false},

    // ============================================================
    // suspended extensions
    // ============================================================

    {M5(0,2,5,7,10),      "9sus4",            "9sus4",      true,  false},
    {M6(0,2,5,7,9,10),    "13sus4",           "13sus4",     true,  false},
    {M6(0,2,5,7,6,10),    "9sus4(#11)",       "9sus4#11",   true,  false},

    {M5(0,2,7,9,10),      "9sus2",            "9sus2",      true,  false},
    {M6(0,2,7,9,10,5),    "13sus2",           "13sus2",     true,  false},

    {M6(0,2,4,5,7,10),    "11sus",            "11sus",      true,  false},
    {M7(0,2,5,7,9,10,6),  "13sus4(#11)",      "13sus4#11",  true,  false},

    // ============================================================
    // augmented family
    // ============================================================

    {M5(0,2,4,8,10),      "aug9",             "+9",         true,  false},
    {M5(0,2,4,8,11),      "augMaj9",          "+maj9",      true,  false},
    {M6(0,2,4,5,8,11),    "augMaj11",         "+maj11",     true,  false},
    {M7(0,2,4,5,8,9,11),  "augMaj13",         "+maj13",     true,  false},

    {M6(0,2,4,8,9,10),    "aug13",            "+13",        true,  false},
    {M6(0,4,6,8,10,11),   "augMaj7(#11)",     "+maj7#11",   true,  false},

    // ============================================================
    // diminished family
    // ============================================================

    {M4(0,3,6,11),         "dimMaj7",          "dimMaj7",    false, true},
    {M5(0,2,3,6,9),        "dim9",             "dim9",       false, true},
    {M5(0,1,3,6,9),        "dim(b9)",          "dimb9",      false, true},
    {M6(0,2,3,5,6,9),      "dim11",            "dim11",      false, true},
    {M7(0,2,3,5,6,8,9),    "dim13",            "dim13",      false, true},

    {M5(0,2,3,6,10),       "hdim9",            "ø9",         false, true},
    {M6(0,2,3,5,6,10),     "hdim11",           "ø11",        false, true},
    {M7(0,2,3,5,6,8,10),   "hdim13",           "ø13",        false, true},

    // ============================================================
    // diminished variants with major 7
    // ============================================================

    {M5(0,2,3,6,11),        "dimMaj9",         "dimMaj9",    false, true},
    {M6(0,2,3,5,6,11),      "dimMaj11",        "dimMaj11",   false, true},
    {M7(0,2,3,5,6,9,11),    "dimMaj13",        "dimMaj13",   false, true},

};*/
static const uint8_t NUM_TEMPLATES = sizeof(TEMPLATES) / sizeof(TEMPLATES[0]);

// Alternative candidates may omit at most this many played pitch classes.
// Extra chord tones are not allowed: alternatives must be subsets of the
// currently held notes.
#ifndef MAX_ALTERNATIVE_MISSING_NOTES
#define MAX_ALTERNATIVE_MISSING_NOTES 1
#endif

#ifndef MAX_ALTERNATIVE_EXTRA_NOTES
#define MAX_ALTERNATIVE_EXTRA_NOTES 0
#endif

static uint16_t rotateRight12(uint16_t mask, uint8_t root) {
    return ((mask >> root) | (mask << (12 - root))) & 0x0FFF;
}

static uint8_t popcount12(uint16_t v) {
    uint8_t c = 0;
    for (uint8_t i = 0; i < 12; i++) if (v & (1 << i)) c++;
    return c;
}

struct ScoredChord {
    ChordResult result;
    int score;
    uint8_t missing;
    uint8_t extra;
};

static bool shouldComeBefore(const ScoredChord& candidate,
                             const ScoredChord& current) {
    const bool candidateExact = candidate.missing == 0 && candidate.extra == 0;
    const bool currentExact = current.missing == 0 && current.extra == 0;
    if (candidateExact != currentExact) {
        return candidateExact;
    }
    if (candidate.score != current.score) {
        return candidate.score > current.score;
    }
    if (candidate.result.rootPc == candidate.result.bassPc &&
        current.result.rootPc != current.result.bassPc) {
        return true;
    }
    return false;
}

uint8_t detectChordCandidates(uint16_t noteMask, uint8_t lowestPc,
                              ChordResult* results, uint8_t maxResults) {
    if (results == nullptr || maxResults == 0 || noteMask == 0) {
        return 0;
    }

    ScoredChord ranked[NUM_TEMPLATES * 12];
    uint16_t rankedCount = 0;

    for (uint8_t root = 0; root < 12; root++) {
        if (!(noteMask & (1 << root))) continue;
        uint16_t rotated = rotateRight12(noteMask, root);

        for (uint8_t t = 0; t < NUM_TEMPLATES; t++) {
            uint16_t tmpl = TEMPLATES[t].mask;
            uint8_t matched = popcount12(rotated & tmpl);
            uint8_t missing = popcount12(tmpl & ~rotated);
            uint8_t extra   = popcount12(rotated & ~tmpl);
            int score = matched * 3 - missing * 2 - extra * 2;

            ranked[rankedCount++] = {
                {root, lowestPc, TEMPLATES[t].fullName, TEMPLATES[t].shorthand,
                 TEMPLATES[t].isMajorFamily, TEMPLATES[t].isDiminished, true},
                score,
                missing,
                extra
            };
        }
    }

    for (uint16_t i = 1; i < rankedCount; ++i) {
        ScoredChord candidate = ranked[i];
        uint16_t position = i;
        while (position > 0 && shouldComeBefore(candidate, ranked[position - 1])) {
            ranked[position] = ranked[position - 1];
            --position;
        }
        ranked[position] = candidate;
    }

    uint8_t resultCount = 0;
    for (uint16_t i = 0; i < rankedCount && resultCount < maxResults; ++i) {
        if (ranked[i].missing > MAX_ALTERNATIVE_MISSING_NOTES ||
            ranked[i].extra > MAX_ALTERNATIVE_EXTRA_NOTES) {
            continue;
        }
        results[resultCount++] = ranked[i].result;
    }
    return resultCount;
}

ChordResult detectChord(uint16_t noteMask, uint8_t lowestPc) {
    ChordResult best = {0, lowestPc, "unknown", "?", true, false, false};
    if (detectChordCandidates(noteMask, lowestPc, &best, 1) > 0) {
        return best;
    }
    return best;
}