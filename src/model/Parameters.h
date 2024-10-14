#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace tonalizer {

namespace ParamIDs {
    // --- Scale / tonal targeting --------------------------------------------
    inline constexpr const char* rootNote         = "rootNote";
    inline constexpr const char* scaleType        = "scaleType";

    // --- Autotune (spectral morpher) ----------------------------------------
    inline constexpr const char* retuneSpeed      = "retuneSpeed";
    inline constexpr const char* correctionAmount = "correctionAmount";
    inline constexpr const char* crossfade        = "crossfade";

    // --- Legacy eraser surface (still drives old editor knobs) --------------
    inline constexpr const char* eraseAmount      = "eraseAmount";
    inline constexpr const char* eraseSharpness   = "eraseSharpness";

    // --- New detection engine -----------------------------------------------
    inline constexpr const char* depth            = "depth";
    inline constexpr const char* threshold        = "threshold";
    inline constexpr const char* ratio            = "ratio";
    inline constexpr const char* knee             = "knee";
    inline constexpr const char* selectivity      = "selectivity";
    inline constexpr const char* softHard         = "softHard";
    inline constexpr const char* attackMs         = "attackMs";
    inline constexpr const char* releaseMs        = "releaseMs";
    inline constexpr const char* crossfeed        = "crossfeed";
    inline constexpr const char* stereoLink       = "stereoLink";
    inline constexpr const char* stereoMode       = "stereoMode"; // 0=LR, 1=MS
    inline constexpr const char* sidechainOn      = "sidechainOn";

    // Quality tier (live + offline). Indexes into:
    //   0 Eco (1024) / 1 Low (2048) / 2 Standard (4096)
    //   3 High (8192) / 4 Insane (16384)
    inline constexpr const char* quality        = "quality";
    inline constexpr const char* offlineQuality = "offlineQuality";

    // --- IO -----------------------------------------------------------------
    inline constexpr const char* dryWet           = "dryWet";
    inline constexpr const char* delta            = "delta";
    inline constexpr const char* bypass           = "bypass";

    // --- Bands (6x: freq / q / gainDb / on) ---------------------------------
    inline constexpr const char* kBandFreqIds[6] = {
        "band0_freq","band1_freq","band2_freq","band3_freq","band4_freq","band5_freq"
    };
    inline constexpr const char* kBandQIds[6] = {
        "band0_q","band1_q","band2_q","band3_q","band4_q","band5_q"
    };
    inline constexpr const char* kBandGainIds[6] = {
        "band0_gain","band1_gain","band2_gain","band3_gain","band4_gain","band5_gain"
    };
    inline constexpr const char* kBandOnIds[6] = {
        "band0_on","band1_on","band2_on","band3_on","band4_on","band5_on"
    };
    inline const char* bandFreq(int i) { return kBandFreqIds[i]; }
    inline const char* bandQ   (int i) { return kBandQIds[i]; }
    inline const char* bandGain(int i) { return kBandGainIds[i]; }
    inline const char* bandOn  (int i) { return kBandOnIds[i]; }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace tonalizer
