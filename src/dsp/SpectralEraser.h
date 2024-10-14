#pragma once
#include "dsp/DetectionEngine.h"
#include <vector>

namespace tonalizer {

class StereoLinker;
class BandOverlay;

// Thin wrapper over DetectionEngine that preserves the old SpectralEraser
// interface used by SpectralMorpher while exposing the new detection pipeline.
class SpectralEraser
{
public:
    SpectralEraser() = default;

    void prepare(int numBins, double sampleRate, int hopSize);
    void reset();

    // Legacy config (called when key/scale changes).
    void setAtonality(const float* atonality, int numBins);

    // Legacy 2-arg form kept for back-compat; maps eraseAmount -> depth,
    // sharpness -> soft/hard & selectivity so the old UI still does something.
    void setParameters(float eraseAmountPct, float sharpnessPct);

    // New, preferred API.
    void setDetectionParams(const DetectionEngine::Params& p) { engine_.setParams(p); }

    // Stereo + bands
    void setChannelIndex(int ch) { channel_ = ch; }
    void setStereoLinker(StereoLinker* linker) { linker_ = linker; }
    void setBandOverlay(const BandOverlay* bands) { bands_ = bands; }

    // External detection source (sidechain magnitudes). Pass nullptr/0 to
    // fall back to using the input magnitudes for detection.
    void setExternalDetection(const float* mag, int numBins);

    // Process magnitudes in-place (mag -> mag * gain).
    void process(float* magnitudes, int numBins);

    const float* getGains() const { return gains_.data(); }

private:
    DetectionEngine engine_;
    std::vector<float> gains_;
    std::vector<float> externalDetection_;
    bool hasExternalDetection_ = false;
    int numBins_ = 0;
    int channel_ = 0;
    StereoLinker* linker_ = nullptr;
    const BandOverlay* bands_ = nullptr;
};

} // namespace tonalizer
