#include "dsp/SpectralEraser.h"
#include "dsp/BandOverlay.h"
#include "dsp/StereoLinker.h"
#include <algorithm>

namespace tonalizer {

void SpectralEraser::prepare(int numBins, double sampleRate, int hopSize)
{
    numBins_ = numBins;
    gains_.assign(static_cast<size_t>(numBins), 1.0f);
    externalDetection_.assign(static_cast<size_t>(numBins), 0.0f);
    hasExternalDetection_ = false;
    engine_.prepare(numBins, sampleRate, hopSize);
}

void SpectralEraser::reset()
{
    std::fill(gains_.begin(), gains_.end(), 1.0f);
    std::fill(externalDetection_.begin(), externalDetection_.end(), 0.0f);
    hasExternalDetection_ = false;
    engine_.reset();
}

void SpectralEraser::setExternalDetection(const float* mag, int numBins)
{
    if (mag == nullptr || numBins <= 0) { hasExternalDetection_ = false; return; }
    const int n = std::min(numBins, static_cast<int>(externalDetection_.size()));
    for (int i = 0; i < n; ++i) externalDetection_[static_cast<size_t>(i)] = mag[i];
    hasExternalDetection_ = true;
}

void SpectralEraser::setAtonality(const float* atonality, int numBins)
{
    engine_.setAtonalityBias(atonality, numBins);
}

void SpectralEraser::setParameters(float eraseAmountPct, float sharpnessPct)
{
    DetectionEngine::Params p;
    p.depth       = std::clamp(eraseAmountPct / 100.0f, 0.0f, 1.0f);
    // Map old "sharpness" both to softHard (curve shape) and selectivity
    // (how much atonal bias dominates) so legacy presets keep roughly working.
    const float s = std::clamp(sharpnessPct / 100.0f, 0.0f, 1.0f);
    p.softHard    = s;
    p.selectivity = 0.25f + 0.5f * s;
    p.thresholdDb = -6.0f;
    p.ratio       = 4.0f;
    p.kneeDb      = 6.0f;
    p.attackMs    = 3.0f;
    p.releaseMs   = 50.0f;
    p.crossfeed   = 0.0f;
    engine_.setParams(p);
}

void SpectralEraser::process(float* magnitudes, int numBins)
{
    if (magnitudes == nullptr || numBins <= 0) return;
    const int n = std::min(numBins, numBins_);

    if (bands_ != nullptr && bands_->getNumBins() == n)
        engine_.setBandCurve(bands_->getCurve(), n);

    if (linker_ != nullptr)
    {
        if (const float* peer = linker_->peer(channel_))
            engine_.setPeerMagnitude(peer, n);
    }

    // Detection source: external sidechain if provided, else the input mags.
    const float* detectionSrc = hasExternalDetection_
        ? externalDetection_.data()
        : magnitudes;

    engine_.process(detectionSrc, gains_.data(), n);

    for (int i = 0; i < n; ++i)
        magnitudes[i] *= gains_[static_cast<size_t>(i)];

    if (linker_ != nullptr)
        linker_->publish(channel_, engine_.getDetectionMagnitude(), n);
}

} // namespace tonalizer
