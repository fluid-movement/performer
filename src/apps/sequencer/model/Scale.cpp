#include "Scale.h"
#include "UserScale.h"

#define ARRAY_SIZE(_array_) (sizeof(_array_) / sizeof(_array_[0]))
#define NOTE_SCALE(_name_, _title_, _chromatic_, ...) \
static const uint16_t _name_##_notes[] = { __VA_ARGS__ }; \
static const NoteScale _name_(_title_, _chromatic_, ARRAY_SIZE(_name_##_notes), _name_##_notes);

// Diatonic modes (modes of the major scale)
NOTE_SCALE(ionianScale,     "Maj: Ionian",    true, 0, 256, 512, 640, 896, 1152, 1408)
NOTE_SCALE(dorianScale,     "Maj: Dorian",    true, 0, 256, 384, 640, 896, 1152, 1280)
NOTE_SCALE(phrygianScale,   "Maj: Phrygian",  true, 0, 128, 384, 640, 896, 1024, 1280)
NOTE_SCALE(lydianScale,     "Maj: Lydian",    true, 0, 256, 512, 768, 896, 1152, 1408)
NOTE_SCALE(mixolydianScale, "Maj: Mixolyd.",  true, 0, 256, 512, 640, 896, 1152, 1280)
NOTE_SCALE(aeolianScale,    "Maj: Aeolian",   true, 0, 256, 384, 640, 896, 1024, 1280)
NOTE_SCALE(locrianScale,    "Maj: Locrian",   true, 0, 128, 384, 640, 768, 1024, 1280)

// Harmonic minor modes
NOTE_SCALE(hmHarmonicMinScale, "HM: Harm. Min",  true, 0, 256, 384, 640, 896, 1024, 1408)
NOTE_SCALE(hmLocrianN6Scale,   "HM: Locrian n6", true, 0, 128, 384, 640, 768, 1152, 1280)
NOTE_SCALE(hmIonianS5Scale,    "HM: Ionian #5",  true, 0, 256, 512, 640, 1024, 1152, 1408)
NOTE_SCALE(hmDorianS4Scale,    "HM: Dorian #4",  true, 0, 256, 384, 768, 896, 1152, 1280)
NOTE_SCALE(hmPhrygDomScale,    "HM: Phryg Dom",  true, 0, 128, 512, 640, 896, 1024, 1280)
NOTE_SCALE(hmLydianS2Scale,    "HM: Lydian #2",  true, 0, 384, 512, 768, 896, 1152, 1408)
NOTE_SCALE(hmSupLocB7Scale,    "HM: Sup Loc b7", true, 0, 128, 384, 512, 768, 1024, 1152)

// Melodic minor modes (ascending form)
NOTE_SCALE(mmMelodicMinScale,  "MM: Melod. Min", true, 0, 256, 384, 640, 896, 1152, 1408)
NOTE_SCALE(mmDorianB2Scale,    "MM: Dorian b2",  true, 0, 128, 384, 640, 896, 1152, 1280)
NOTE_SCALE(mmLydianAugScale,   "MM: Lydian Aug", true, 0, 256, 512, 768, 1024, 1152, 1408)
NOTE_SCALE(mmLydianDomScale,   "MM: Lydian Dom", true, 0, 256, 512, 768, 896, 1152, 1280)
NOTE_SCALE(mmMixoB6Scale,      "MM: Mixo b6",    true, 0, 256, 512, 640, 896, 1024, 1280)
NOTE_SCALE(mmLocrianN2Scale,   "MM: Locrian n2", true, 0, 256, 384, 640, 768, 1024, 1280)
NOTE_SCALE(mmAlteredScale,     "MM: Altered",    true, 0, 128, 384, 512, 768, 1024, 1280)

#undef ARRAY_SIZE
#undef NOTE_SCALE

static const Scale *scales[] = {
    // Diatonic modes
    &ionianScale,
    &dorianScale,
    &phrygianScale,
    &lydianScale,
    &mixolydianScale,
    &aeolianScale,
    &locrianScale,

    // Harmonic minor modes
    &hmHarmonicMinScale,
    &hmLocrianN6Scale,
    &hmIonianS5Scale,
    &hmDorianS4Scale,
    &hmPhrygDomScale,
    &hmLydianS2Scale,
    &hmSupLocB7Scale,

    // Melodic minor modes
    &mmMelodicMinScale,
    &mmDorianB2Scale,
    &mmLydianAugScale,
    &mmLydianDomScale,
    &mmMixoB6Scale,
    &mmLocrianN2Scale,
    &mmAlteredScale,
};

static const int BuiltinCount = sizeof(scales) / sizeof(Scale *);
static const int UserCount = UserScale::userScales.size();

int Scale::Count = BuiltinCount + UserCount;

const Scale &Scale::get(int index) {
    if (index < BuiltinCount) {
        return *scales[index];
    } else {
        return UserScale::userScales[index - BuiltinCount];
    }
}

const char *Scale::name(int index) {
    if (index < BuiltinCount) {
        return get(index).displayName();
    } else {
        switch (index - BuiltinCount) {
        case 0: return "User1";
        case 1: return "User2";
        case 2: return "User3";
        case 3: return "User4";
        }
    }
    return nullptr;
}
