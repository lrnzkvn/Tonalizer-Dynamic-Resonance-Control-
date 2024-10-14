#pragma once
#include <array>
#include <vector>

namespace tonalizer {

// Six Soothe-style bands that BIAS DETECTION (not a post-EQ).
// Each band's per-bin dB contribution is summed; the resulting curve is
// added to salienceDb inside DetectionEngine. Boost => detect harder at
// that frequency; cut => ignore resonances there.
class BandOverlay
{
public:
    static constexpr int kNumBands = 6;

    struct Band
    {
        bool  on    = false;
        float freq  = 1000.0f;   // Hz
        float q     = 2.0f;
        float gainDb = 0.0f;     // +/- dB bias at centre bin
    };

    void prepare(int numBins, double sampleRate);
    void setBand(int index, const Band& b);
    void recompute();

    const float* getCurve() const { return curve_.data(); }
    int getNumBins() const { return static_cast<int>(curve_.size()); }

private:
    std::array<Band, kNumBands> bands_ {};
    std::vector<float> curve_;
    double sampleRate_ = 44100.0;
    int numBins_ = 0;
};

} // namespace tonalizer
