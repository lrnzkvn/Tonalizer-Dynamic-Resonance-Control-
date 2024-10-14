#pragma once
#include <array>
#include <cmath>
#include <vector>
#include <string>

namespace tonalizer {

class MusicalScale
{
public:
    enum class ScaleType
    {
        // Western Modes (7)
        Major = 0,
        Dorian,
        Phrygian,
        Lydian,
        Mixolydian,
        Minor,
        Locrian,

        // Minor Variants (6)
        HarmonicMinor,
        MelodicMinor,
        HarmonicMajor,
        HungarianMinor,
        NeapolitanMinor,
        NeapolitanMajor,

        // Pentatonic & Blues (4)
        MajorPentatonic,
        MinorPentatonic,
        Blues,
        BluesMajor,

        // Jazz (7)
        BebopDominant,
        BebopMajor,
        Altered,
        LydianDominant,
        DiminishedHW,
        DiminishedWH,
        Augmented,

        // Middle Eastern (4)
        DoubleHarmonic,
        PhrygianDominant,
        Hijaz,
        Persian,

        // East Asian (5)
        Hirajoshi,
        InSen,
        Iwato,
        Kumoi,
        BalinesePelog,

        // Indian (3)
        Todi,
        Marwa,
        Purvi,

        // African (1)
        Egyptian,

        // Experimental (4)
        WholeTone,
        Prometheus,
        Tritone,
        Enigmatic,

        // Special (1)
        Chromatic,

        NumScales
    };

    static constexpr int kNumScales = static_cast<int>(ScaleType::NumScales);

    MusicalScale() = default;
    MusicalScale(int rootNote, ScaleType type);

    // Returns tonal distance in [0, 1] for a given pitch class (0-11).
    // 0.0 = perfectly in-scale, 1.0 = maximally atonal (half-step from any scale tone).
    float tonalDistance(int pitchClass) const;

    // Returns tonal distance for a frequency in Hz.
    // Returns 0.0 for frequencies outside 20Hz-20kHz.
    float tonalDistanceForFreq(float freqHz) const;

    // Find the nearest scale pitch (as a MIDI note number) to a given frequency.
    // Returns 0.0f if freq is outside valid range.
    float findNearestScalePitch(float freqHz) const;

    // Returns true if the given pitch class (0-11) is in the current scale.
    bool isInScale(int pitchClass) const;

    int getRootNote() const { return rootNote_; }
    ScaleType getScaleType() const { return scaleType_; }

    static const std::array<bool, 12>& getScalePattern(ScaleType type);

    // Returns display names for all 42 scales.
    static const std::vector<std::string>& getScaleNames();

private:
    int rootNote_ = 0;
    ScaleType scaleType_ = ScaleType::Major;
    std::array<float, 12> distances_ {};
    std::array<bool, 12> rotatedPattern_ {}; // pattern rotated to absolute pitch classes

    void computeDistances();
};

} // namespace tonalizer
