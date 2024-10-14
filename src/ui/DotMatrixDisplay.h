#pragma once

#include <array>

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

namespace tonalizer {

class DotMatrixDisplay : public juce::Component,
                         private juce::Timer
{
public:
    explicit DotMatrixDisplay(TonalizerProcessor& processor);

    void paint(juce::Graphics& g) override;

private:
    static constexpr int kNumBins = SpectralMorpher::kMaxNumBins;

    void timerCallback() override;
    float frequencyToX(float frequency, juce::Rectangle<float> plotArea) const;
    float decibelsToY(float db, juce::Rectangle<float> plotArea) const;
    float sampleSpectrum(const std::array<float, kNumBins>& values, float bin, int radius) const;

    TonalizerProcessor& processor_;
    std::array<float, kNumBins> rawSpectrum_ {};
    std::array<float, kNumBins> rawSpectrumCh1_ {};
    std::array<float, kNumBins> processedSpectrum_ {};
    int currentFFTSize_ = 2048;
    int currentNumBins_ = 1025;
    float meterLevel_ = 0.0f;
    float crossfadeAmount_ = 0.0f;
    bool hasCh1_ = false;
    bool msMode_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DotMatrixDisplay)
};

} // namespace tonalizer
