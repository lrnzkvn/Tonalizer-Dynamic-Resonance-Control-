#pragma once
#include <vector>

namespace tonalizer {

// Soothe-style spectral resonance detector.
// Pipeline: mag -> envelope -> salience -> atonal/band bias -> gain computer ->
//           freq smoothing -> per-bin A/R smoothing -> output gain mask.
class DetectionEngine
{
public:
    struct Params
    {
        float depth       = 0.20f;   // 0..1, overall suppression strength
        float thresholdDb = -6.0f;   // dB salience above envelope before reducing
        float ratio       = 4.0f;    // >= 1
        float kneeDb      = 6.0f;    // soft knee width in dB (0 = hard)
        float selectivity = 0.5f;    // 0 = pure peak salience, 1 = pure atonal bias
        float softHard    = 0.5f;    // 0..1, shapes the reduction curve exponent
        float attackMs    = 3.0f;
        float releaseMs   = 50.0f;
        float crossfeed   = 0.0f;    // 0..1 stereo link strength
    };

    DetectionEngine() = default;

    void prepare(int numBins, double sampleRate, int hopSize);
    void reset();

    void setParams(const Params& p);
    void setAtonalityBias(const float* atonality, int numBins);
    void setBandCurve(const float* bandDbCurve, int numBins);
    void setPeerMagnitude(const float* peerMag, int numBins);

    // Runs the full detector; writes per-bin gains into outGains.
    // inputMag is the "self" magnitude (used for detection and as the spectrum to be gated).
    void process(const float* inputMag, float* outGains, int numBins);

    const float* getDetectionMagnitude() const { return detectionMag_.data(); }

private:
    Params params_ {};
    double sampleRate_ = 44100.0;
    int hopSize_ = 256;

    // Precomputed log-freq box half-widths per bin (1/3-octave envelope).
    std::vector<int> envHalfWidth_;

    std::vector<float> atonality_;
    std::vector<float> bandDb_;
    std::vector<float> peerMag_;
    bool hasPeer_ = false;

    std::vector<float> detectionMag_;
    std::vector<float> prefixSum_;   // size n+1; prefixSum_[k] = sum of detectionMag_[0..k-1]
    std::vector<float> envelope_;
    std::vector<float> targetGain_;
    std::vector<float> freqSmoothedGain_;
    std::vector<float> smoothedGain_;

    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;

    void recomputeCoeffs();
};

} // namespace tonalizer
