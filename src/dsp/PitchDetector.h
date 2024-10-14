#pragma once
#include <vector>

namespace tonalizer {

class PitchDetector
{
public:
    PitchDetector() = default;

    void prepare(double sampleRate);
    void reset();

    // Detect pitch from a window of samples. Returns frequency in Hz, or 0 if unvoiced.
    // windowSize should be 2048 (matches STFT frame size).
    float detect(const float* samples, int windowSize);

    // Confidence of last detection [0, 1]. < 0.5 means unreliable.
    float getConfidence() const { return confidence_; }

private:
    double sampleRate_ = 44100.0;
    float confidence_ = 0.0f;

    // YIN parameters
    static constexpr float kThreshold = 0.15f;
    static constexpr float kMinFreq = 60.0f;
    static constexpr float kMaxFreq = 1200.0f;

    // Working buffers
    std::vector<float> yinBuffer_;
};

} // namespace tonalizer
