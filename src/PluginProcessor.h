#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/SpectralMorpher.h"
#include "dsp/SpectralEraser.h"
#include "dsp/BandOverlay.h"
#include "dsp/StereoLinker.h"
#include "model/MusicalScale.h"
#include "model/TonalClassifier.h"
#include "model/Parameters.h"
#include <array>
#include <mutex>
#include <atomic>

namespace tonalizer {

class TonalizerProcessor : public juce::AudioProcessor,
                        private juce::AsyncUpdater,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    TonalizerProcessor();
    ~TonalizerProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    juce::Point<int> getStoredEditorSize() const;
    void setStoredEditorSize(juce::Point<int> size);

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    struct VisualizationData
    {
        // Sized to the max possible FFT; only the first numBins entries are valid.
        // magnitudes / magnitudesCh1 are Mid/Side when msMode, else L/R.
        std::array<float, SpectralMorpher::kMaxNumBins> magnitudes {};
        std::array<float, SpectralMorpher::kMaxNumBins> magnitudesCh1 {};
        std::array<float, SpectralMorpher::kMaxNumBins> gains {};
        std::array<float, SpectralMorpher::kMaxNumBins> atonality {};
        int numBins = 0;
        int fftSize = 0;
        int numChannels = 1;
        bool msMode = false;
        bool ready = false;
    };

    VisualizationData getVisualizationSnapshot() const;
    SpectralMorpher::PitchInfo getAutotunePitchInfo() const;

private:
    static constexpr int kMaxChannels = 2;

    // Cached parameter pointers.
    juce::AudioParameterChoice* rootNoteParam = nullptr;
    juce::AudioParameterChoice* scaleTypeParam = nullptr;
    juce::AudioParameterFloat*  eraseAmountParam = nullptr;
    juce::AudioParameterFloat*  sharpnessParam = nullptr;
    juce::AudioParameterFloat*  dryWetParam = nullptr;
    juce::AudioParameterFloat*  crossfadeParam = nullptr;
    juce::AudioParameterFloat*  retuneSpeedParam = nullptr;
    juce::AudioParameterFloat*  correctionAmountParam = nullptr;
    juce::AudioParameterBool*   bypassParam = nullptr;
    juce::AudioParameterBool*   deltaParam = nullptr;

    // New detection params.
    juce::AudioParameterFloat* depthParam = nullptr;
    juce::AudioParameterFloat* thresholdParam = nullptr;
    juce::AudioParameterFloat* ratioParam = nullptr;
    juce::AudioParameterFloat* kneeParam = nullptr;
    juce::AudioParameterFloat* selectivityParam = nullptr;
    juce::AudioParameterFloat* softHardParam = nullptr;
    juce::AudioParameterFloat* attackParam = nullptr;
    juce::AudioParameterFloat* releaseParam = nullptr;
    juce::AudioParameterFloat* crossfeedParam = nullptr;
    juce::AudioParameterBool*   stereoLinkParam = nullptr;
    juce::AudioParameterChoice* stereoModeParam = nullptr;
    juce::AudioParameterBool*   sidechainOnParam = nullptr;
    juce::AudioParameterChoice* qualityParam = nullptr;
    juce::AudioParameterChoice* offlineQualityParam = nullptr;

    // Band params (6 groups).
    struct BandParams {
        juce::AudioParameterFloat* freq = nullptr;
        juce::AudioParameterFloat* q = nullptr;
        juce::AudioParameterFloat* gain = nullptr;
        juce::AudioParameterBool*  on = nullptr;
    };
    std::array<BandParams, BandOverlay::kNumBands> bandParams_ {};

    // DSP
    std::array<SpectralMorpher, kMaxChannels> morphers_;
    std::array<SpectralEraser, kMaxChannels> erasers_;
    TonalClassifier classifier_;
    MusicalScale currentScale_;
    BandOverlay bands_;
    StereoLinker linker_;

    // Buffers
    juce::AudioBuffer<float> dryBuffer_;

    double currentSampleRate_ = 44100.0;

    int lastRootNote_ = -1;
    int lastScaleType_ = -1;
    int lastFFTOrder_ = -1;
    int lastMaxBlockSize_ = 0;
    std::array<BandOverlay::Band, BandOverlay::kNumBands> lastBandState_ {};

    void updateScaleIfNeeded();
    void updateBandsIfNeeded();
    void pushDetectionParams();
    void applyQuality(int fftOrder);
    int qualityIndexToFFTOrder(int idx) const;

    // Async quality swap plumbing.
    void handleAsyncUpdate() override;
    void parameterChanged(const juce::String& id, float newValue) override;
    std::atomic<int> pendingFFTOrder_ { -1 };
    std::atomic<bool> lastOfflineFlag_ { false };

    mutable std::mutex vizMutex_;
    VisualizationData vizData_;

    // Per-block ramping for click-prone detection params. Re-target each block
    // from the APVTS value, then read the smoothed snapshot. Per-block (not
    // per-sample) is fine: the detector only re-evaluates per FFT hop.
    juce::SmoothedValue<float> smDepth_       { 0.0f };
    juce::SmoothedValue<float> smThresholdDb_ { 0.0f };
    juce::SmoothedValue<float> smSelectivity_ { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TonalizerProcessor)
};

} // namespace tonalizer
