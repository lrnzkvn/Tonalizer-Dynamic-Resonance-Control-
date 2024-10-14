#include "model/Parameters.h"
#include "model/MusicalScale.h"

namespace tonalizer {

namespace {
    juce::NormalisableRange<float> logRange(float lo, float hi)
    {
        juce::NormalisableRange<float> r(lo, hi);
        r.setSkewForCentre(std::sqrt(lo * hi));
        return r;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Tonal targeting ----------------------------------------------------
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::rootNote, 1 },
        "Root Note",
        juce::StringArray { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" },
        0));

    juce::StringArray scaleNames;
    for (const auto& name : MusicalScale::getScaleNames())
        scaleNames.add(juce::String(name));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::scaleType, 1 }, "Scale", scaleNames, 0));

    // --- Legacy eraser surface (kept so existing editor works) --------------
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::eraseAmount, 1 }, "Erase Amount",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::eraseSharpness, 1 }, "Sharpness",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // --- Detection engine ---------------------------------------------------
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::depth, 1 }, "Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::threshold, 1 }, "Threshold",
        juce::NormalisableRange<float>(-40.0f, 12.0f, 0.1f), -6.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::ratio, 1 }, "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.01f, 0.5f), 4.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::knee, 1 }, "Knee",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 6.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::selectivity, 1 }, "Selectivity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::softHard, 1 }, "Soft / Hard",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::attackMs, 1 }, "Attack",
        juce::NormalisableRange<float>(0.1f, 200.0f, 0.01f, 0.35f), 3.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::releaseMs, 1 }, "Release",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 0.1f, 0.35f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::crossfeed, 1 }, "Stereo Crossfeed",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::stereoLink, 1 }, "Stereo Link", true));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::stereoMode, 1 }, "Stereo Mode",
        juce::StringArray { "L/R", "M/S" }, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::sidechainOn, 1 }, "Sidechain", false));

    const juce::StringArray qualityNames {
        "Eco (1024)", "Low (2048)", "Standard (4096)", "High (8192)", "Insane (16384)"
    };
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::quality, 1 }, "Quality", qualityNames, 1)); // 2048
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::offlineQuality, 1 }, "Offline Quality",
        qualityNames, 3)); // 8192 for bounces

    // --- Autotune -----------------------------------------------------------
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::retuneSpeed, 1 }, "Retune Speed",
        juce::NormalisableRange<float>(0.0f, 400.0f, 1.0f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::correctionAmount, 1 }, "Correction",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::crossfade, 1 }, "Crossfade",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // --- IO -----------------------------------------------------------------
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::dryWet, 1 }, "Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::delta, 1 }, "Delta", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::bypass, 1 }, "Bypass", false));

    // --- Bands (6) ----------------------------------------------------------
    const float defaultFreqs[6] = { 100.f, 300.f, 800.f, 2000.f, 5000.f, 10000.f };
    for (int i = 0; i < 6; ++i)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::bandFreq(i), 1 },
            "Band " + juce::String(i + 1) + " Freq",
            logRange(20.0f, 20000.0f), defaultFreqs[i],
            juce::AudioParameterFloatAttributes().withLabel("Hz")));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::bandQ(i), 1 },
            "Band " + juce::String(i + 1) + " Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 2.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::bandGain(i), 1 },
            "Band " + juce::String(i + 1) + " Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { ParamIDs::bandOn(i), 1 },
            "Band " + juce::String(i + 1) + " On", false));
    }

    return layout;
}

} // namespace tonalizer
