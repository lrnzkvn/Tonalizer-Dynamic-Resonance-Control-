#pragma once
#include <juce_dsp/juce_dsp.h>
#include "dsp/PitchDetector.h"
#include "dsp/SpectralEraser.h"
#include "model/MusicalScale.h"
#include <vector>
#include <atomic>
#include <memory>

namespace tonalizer {

class SpectralMorpher
{
public:
    // Max bounds for buffer allocation / viz payload. Actual runtime sizes are
    // chosen per prepare() call via the quality tier.
    static constexpr int kMaxFFTOrder = 14;            // 16384 samples
    static constexpr int kMaxFFTSize  = 1 << kMaxFFTOrder;
    static constexpr int kMaxNumBins  = kMaxFFTSize / 2 + 1;

    // Legacy aliases used by call sites that expected compile-time constants.
    // These are the *upper bounds*; runtime sizes are reachable via the
    // instance getters getFFTSize()/getHopSize()/getNumBins().
    static constexpr int kFFTSize = kMaxFFTSize;
    static constexpr int kNumBins = kMaxNumBins;
    static constexpr int kHopSize = kMaxFFTSize / 4;

    SpectralMorpher();
    ~SpectralMorpher();

    void prepare(double sampleRate, int maxBlockSize);
    void prepare(double sampleRate, int maxBlockSize, int fftOrder);
    void reset();

    void setEraser(SpectralEraser* eraser);
    void setScale(const MusicalScale& scale);
    void setRetuneSpeed(float ms);
    void setCorrectionAmount(float amount);
    void setSpectralCrossfade(float amount01);

    void setSidechainEnabled(bool enabled);
    void setPitchDetectEnabled(bool enabled);
    void setSmoothedRatio(float ratio);
    float getSmoothedRatio() const;

    void processBlock(float* mainData, int numSamples);
    void processBlock(float* mainData, const float* sidechainData, int numSamples);
    float processSample(float inSample, float sidechainSample = 0.0f,
                        bool sidechainValid = false);

    int getFFTSize() const { return fftSize_; }
    int getHopSize() const { return hopSize_; }
    int getNumBins() const { return numBins_; }
    int getFFTOrder() const { return fftOrder_; }
    int getLatency() const { return fftSize_; }

    struct PitchInfo
    {
        float detectedHz = 0.0f;
        float detectedSemitone = 0.0f;
        float targetSemitone = 0.0f;
        float correctedSemitone = 0.0f;
        float confidence = 0.0f;
    };

    PitchInfo getPitchInfo() const;
    const float* getEraseGains() const;
    const float* getMagnitudes() const { return magnitudes_.data(); }

private:
    std::unique_ptr<juce::dsp::FFT> fft_;
    // Dedicated FFT for the sidechain transform. Same order as fft_, but kept
    // independent so the two transforms never share scratch state — cheap
    // insurance against future overlap (e.g. multithreaded analysis paths).
    std::unique_ptr<juce::dsp::FFT> scFft_;
    double sampleRate_ = 44100.0;

    // Runtime sizing.
    int fftOrder_ = 11;
    int fftSize_ = 2048;
    int hopSize_ = 512;
    int numBins_ = 1025;

    SpectralEraser* eraser_ = nullptr;
    MusicalScale scale_;
    float retuneSpeedMs_ = 50.0f;
    float correctionAmount_ = 1.0f;
    float spectralCrossfade_ = 0.0f;
    bool pitchDetectEnabled_ = true;
    bool sidechainEnabled_ = false;

    std::vector<float> window_;

    std::vector<float> inputBuffer_;
    int inputWritePos_ = 0;
    std::vector<float> outputBuffer_;
    int outputReadPos_ = 0;
    int hopCounter_ = 0;

    std::vector<float> scBuffer_;
    int scWritePos_ = 0;
    bool scHasData_ = false;

    std::vector<float> fftData_;
    std::vector<float> magnitudes_;
    std::vector<float> phases_;

    std::vector<float> scFftData_;
    std::vector<float> scMagnitudes_;

    std::vector<float> eraseMags_;
    std::vector<float> erasePhases_;

    std::vector<float> prevPhases_;
    std::vector<float> synthPhases_;
    std::vector<float> analysisFreqs_;
    std::vector<float> tuneMags_;
    std::vector<float> tunePhases_;
    std::vector<float> shiftedFreqs_;

    std::vector<float> blendedMags_;
    std::vector<float> blendedPhases_;

    std::vector<float> pitchDetectBuffer_;
    PitchDetector pitchDetector_;
    float smoothedRatio_ = 1.0f;

    std::atomic<float> pitchDetectedHz_ { 0.0f };
    std::atomic<float> pitchDetectedSemi_ { 0.0f };
    std::atomic<float> pitchTargetSemi_ { 0.0f };
    std::atomic<float> pitchCorrectedSemi_ { 0.0f };
    std::atomic<float> pitchConfidence_ { 0.0f };

    std::vector<float> eraseGainsCache_;

    void processMainFrame();

    // Hann overlap-add gain for 75% overlap is 2/3.
    static constexpr float kOLAGain = 2.0f / 3.0f;
};

} // namespace tonalizer
