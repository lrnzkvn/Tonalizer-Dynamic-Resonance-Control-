#include "model/MusicalScale.h"
#include <algorithm>
#include <cmath>

namespace tonalizer {

// Scale patterns as pitch-class sets (relative to root = 0)
// Western Modes (7)
static constexpr std::array<bool, 12> kMajor            = {1,0,1,0,1,1,0,1,0,1,0,1};
static constexpr std::array<bool, 12> kDorian           = {1,0,1,1,0,1,0,1,0,1,1,0};
static constexpr std::array<bool, 12> kPhrygian         = {1,1,0,1,0,1,0,1,1,0,1,0};
static constexpr std::array<bool, 12> kLydian           = {1,0,1,0,1,0,1,1,0,1,0,1};
static constexpr std::array<bool, 12> kMixolydian       = {1,0,1,0,1,1,0,1,0,1,1,0};
static constexpr std::array<bool, 12> kMinor            = {1,0,1,1,0,1,0,1,1,0,1,0};
static constexpr std::array<bool, 12> kLocrian          = {1,1,0,1,0,1,1,0,1,0,1,0};

// Minor Variants (6)
static constexpr std::array<bool, 12> kHarmonicMinor    = {1,0,1,1,0,1,0,1,1,0,0,1};
static constexpr std::array<bool, 12> kMelodicMinor     = {1,0,1,1,0,1,0,1,0,1,0,1};
static constexpr std::array<bool, 12> kHarmonicMajor    = {1,0,1,0,1,1,0,1,1,0,0,1};
static constexpr std::array<bool, 12> kHungarianMinor   = {1,0,1,1,0,0,1,1,1,0,0,1};
static constexpr std::array<bool, 12> kNeapolitanMinor  = {1,1,0,1,0,1,0,1,1,0,0,1};
static constexpr std::array<bool, 12> kNeapolitanMajor  = {1,1,0,1,0,1,0,1,0,1,0,1};

// Pentatonic & Blues (4)
static constexpr std::array<bool, 12> kMajorPentatonic  = {1,0,1,0,1,0,0,1,0,1,0,0};
static constexpr std::array<bool, 12> kMinorPentatonic  = {1,0,0,1,0,1,0,1,0,0,1,0};
static constexpr std::array<bool, 12> kBlues            = {1,0,0,1,0,1,1,1,0,0,1,0};
static constexpr std::array<bool, 12> kBluesMajor       = {1,0,1,1,1,0,0,1,0,1,0,0};

// Jazz (7)
static constexpr std::array<bool, 12> kBebopDominant    = {1,0,1,0,1,1,0,1,0,1,1,1};
static constexpr std::array<bool, 12> kBebopMajor       = {1,0,1,0,1,1,0,1,1,1,0,1};
static constexpr std::array<bool, 12> kAltered          = {1,1,0,1,1,0,1,0,1,0,1,0};
static constexpr std::array<bool, 12> kLydianDominant   = {1,0,1,0,1,0,1,1,0,1,1,0};
static constexpr std::array<bool, 12> kDiminishedHW     = {1,1,0,1,1,0,1,1,0,1,1,0};
static constexpr std::array<bool, 12> kDiminishedWH     = {1,0,1,1,0,1,1,0,1,1,0,1};
static constexpr std::array<bool, 12> kAugmented        = {1,0,0,1,1,0,0,1,1,0,0,1};

// Middle Eastern (4)
static constexpr std::array<bool, 12> kDoubleHarmonic   = {1,1,0,0,1,1,0,1,1,0,0,1};
static constexpr std::array<bool, 12> kPhrygianDominant = {1,1,0,0,1,1,0,1,1,0,1,0};
static constexpr std::array<bool, 12> kHijaz            = {1,1,0,0,1,1,0,1,1,0,0,1};
static constexpr std::array<bool, 12> kPersian          = {1,1,0,0,1,1,1,0,1,0,0,1};

// East Asian (5)
static constexpr std::array<bool, 12> kHirajoshi        = {1,0,1,1,0,0,0,1,1,0,0,0};
static constexpr std::array<bool, 12> kInSen            = {1,1,0,0,0,1,0,1,0,0,1,0};
static constexpr std::array<bool, 12> kIwato            = {1,1,0,0,0,1,1,0,0,0,1,0};
static constexpr std::array<bool, 12> kKumoi            = {1,0,1,1,0,0,0,1,0,1,0,0};
static constexpr std::array<bool, 12> kBalinesePelog    = {1,1,0,1,0,0,0,1,1,0,0,0};

// Indian (3)
static constexpr std::array<bool, 12> kTodi             = {1,1,0,1,0,0,1,1,1,0,0,1};
static constexpr std::array<bool, 12> kMarwa            = {1,1,0,0,1,0,1,1,0,1,0,1};
static constexpr std::array<bool, 12> kPurvi            = {1,1,0,0,1,0,1,1,1,0,0,1};

// African (1)
static constexpr std::array<bool, 12> kEgyptian         = {1,0,1,0,0,1,0,1,0,0,1,0};

// Experimental (4)
static constexpr std::array<bool, 12> kWholeTone        = {1,0,1,0,1,0,1,0,1,0,1,0};
static constexpr std::array<bool, 12> kPrometheus       = {1,0,1,0,1,0,1,0,0,1,1,0};
static constexpr std::array<bool, 12> kTritone          = {1,1,0,0,1,0,1,1,0,0,1,0};
static constexpr std::array<bool, 12> kEnigmatic        = {1,1,0,0,1,0,1,0,1,0,1,1};

// Special (1)
static constexpr std::array<bool, 12> kChromatic        = {1,1,1,1,1,1,1,1,1,1,1,1};

static constexpr int kTotalScales = static_cast<int>(MusicalScale::ScaleType::NumScales);

static const std::array<const std::array<bool, 12>*, 42> kAllScalePointers = {
    &kMajor, &kDorian, &kPhrygian, &kLydian, &kMixolydian, &kMinor, &kLocrian,
    &kHarmonicMinor, &kMelodicMinor, &kHarmonicMajor, &kHungarianMinor, &kNeapolitanMinor, &kNeapolitanMajor,
    &kMajorPentatonic, &kMinorPentatonic, &kBlues, &kBluesMajor,
    &kBebopDominant, &kBebopMajor, &kAltered, &kLydianDominant, &kDiminishedHW, &kDiminishedWH, &kAugmented,
    &kDoubleHarmonic, &kPhrygianDominant, &kHijaz, &kPersian,
    &kHirajoshi, &kInSen, &kIwato, &kKumoi, &kBalinesePelog,
    &kTodi, &kMarwa, &kPurvi,
    &kEgyptian,
    &kWholeTone, &kPrometheus, &kTritone, &kEnigmatic,
    &kChromatic
};

const std::array<bool, 12>& MusicalScale::getScalePattern(ScaleType type)
{
    return *kAllScalePointers[static_cast<size_t>(type)];
}

static const std::vector<std::string> kScaleNames = {
    // Western Modes
    "Major (Ionian)", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Minor (Aeolian)", "Locrian",
    // Minor Variants
    "Harmonic Minor", "Melodic Minor", "Harmonic Major", "Hungarian Minor", "Neapolitan Minor", "Neapolitan Major",
    // Pentatonic & Blues
    "Major Pentatonic", "Minor Pentatonic", "Blues", "Blues Major",
    // Jazz
    "Bebop Dominant", "Bebop Major", "Altered", "Lydian Dominant", "Diminished HW", "Diminished WH", "Augmented",
    // Middle Eastern
    "Double Harmonic", "Phrygian Dominant", "Hijaz", "Persian",
    // East Asian
    "Hirajoshi", "In Sen", "Iwato", "Kumoi", "Balinese Pelog",
    // Indian
    "Todi", "Marwa", "Purvi",
    // African
    "Egyptian",
    // Experimental
    "Whole Tone", "Prometheus", "Tritone", "Enigmatic",
    // Special
    "Chromatic"
};

const std::vector<std::string>& MusicalScale::getScaleNames()
{
    return kScaleNames;
}

MusicalScale::MusicalScale(int rootNote, ScaleType type)
    : rootNote_(rootNote), scaleType_(type)
{
    // Pre-compute rotated pattern for fast isInScale / findNearestScalePitch
    const auto& pattern = getScalePattern(scaleType_);
    for (int pc = 0; pc < 12; ++pc)
    {
        int relative = ((pc - rootNote_) % 12 + 12) % 12;
        rotatedPattern_[static_cast<size_t>(pc)] = pattern[static_cast<size_t>(relative)];
    }

    computeDistances();
}

void MusicalScale::computeDistances()
{
    for (int pc = 0; pc < 12; ++pc)
    {
        if (rotatedPattern_[static_cast<size_t>(pc)])
        {
            distances_[static_cast<size_t>(pc)] = 0.0f;
        }
        else
        {
            // Find min distance to nearest scale tone (in semitones)
            int minDist = 12;
            for (int s = 0; s < 12; ++s)
            {
                if (rotatedPattern_[static_cast<size_t>(s)])
                {
                    int d = std::abs(pc - s);
                    d = std::min(d, 12 - d);
                    minDist = std::min(minDist, d);
                }
            }
            // Normalize: max distance is 1 semitone for most scales
            // but could be more for sparse scales. Clamp to [0,1].
            distances_[static_cast<size_t>(pc)] = std::min(1.0f, static_cast<float>(minDist));
        }
    }
}

float MusicalScale::tonalDistance(int pitchClass) const
{
    return distances_[static_cast<size_t>(((pitchClass % 12) + 12) % 12)];
}

float MusicalScale::tonalDistanceForFreq(float freqHz) const
{
    if (freqHz < 20.0f || freqHz > 20000.0f)
        return 0.0f;

    const float midiNote = 69.0f + 12.0f * std::log2(freqHz / 440.0f);

    // Use fractional pitch class for smooth per-bin classification.
    // This avoids hard jumps caused by rounding to the nearest semitone.
    float pitchClass = std::fmod(midiNote, 12.0f);
    if (pitchClass < 0.0f)
        pitchClass += 12.0f;

    float minDist = 12.0f;
    for (int s = 0; s < 12; ++s)
    {
        if (!rotatedPattern_[static_cast<size_t>(s)])
            continue;

        float d = std::abs(pitchClass - static_cast<float>(s));
        d = std::min(d, 12.0f - d); // circular distance on pitch-class ring
        minDist = std::min(minDist, d);
    }

    if (minDist >= 12.0f)
        return 0.0f;

    // Map distance to [0, 1], where >= 1 semitone from any scale tone is "fully atonal".
    return std::clamp(minDist, 0.0f, 1.0f);
}

bool MusicalScale::isInScale(int pitchClass) const
{
    return rotatedPattern_[static_cast<size_t>(((pitchClass % 12) + 12) % 12)];
}

float MusicalScale::findNearestScalePitch(float freqHz) const
{
    if (freqHz < 20.0f || freqHz > 20000.0f)
        return 0.0f;

    float midiNote = 69.0f + 12.0f * std::log2(freqHz / 440.0f);
    int nearestMidi = static_cast<int>(std::round(midiNote));

    // Search up to 6 semitones in both directions for the nearest scale tone
    int bestOffset = 0;
    float bestDist = 100.0f;

    for (int offset = 0; offset <= 6; ++offset)
    {
        // Check above
        int candidateUp = nearestMidi + offset;
        int pcUp = ((candidateUp % 12) + 12) % 12;
        if (rotatedPattern_[static_cast<size_t>(pcUp)])
        {
            float dist = std::abs(midiNote - static_cast<float>(candidateUp));
            if (dist < bestDist)
            {
                bestDist = dist;
                bestOffset = candidateUp;
            }
        }

        // Check below (skip offset=0 since we already checked it)
        if (offset > 0)
        {
            int candidateDown = nearestMidi - offset;
            int pcDown = ((candidateDown % 12) + 12) % 12;
            if (rotatedPattern_[static_cast<size_t>(pcDown)])
            {
                float dist = std::abs(midiNote - static_cast<float>(candidateDown));
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestOffset = candidateDown;
                }
            }
        }
    }

    return static_cast<float>(bestOffset);
}

} // namespace tonalizer
