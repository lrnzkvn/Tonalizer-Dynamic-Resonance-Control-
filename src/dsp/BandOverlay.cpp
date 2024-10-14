#include "dsp/BandOverlay.h"
#include <algorithm>
#include <cmath>

namespace tonalizer {

void BandOverlay::prepare(int numBins, double sampleRate)
{
    numBins_ = std::max(1, numBins);
    sampleRate_ = sampleRate;
    curve_.assign(static_cast<size_t>(numBins_), 0.0f);
    recompute();
}

void BandOverlay::setBand(int index, const Band& b)
{
    if (index < 0 || index >= kNumBands) return;
    bands_[static_cast<size_t>(index)] = b;
}

void BandOverlay::recompute()
{
    if (numBins_ <= 0) return;
    std::fill(curve_.begin(), curve_.end(), 0.0f);
    const float binWidth = static_cast<float>(sampleRate_) /
                           static_cast<float>((numBins_ - 1) * 2);

    for (const auto& band : bands_)
    {
        if (! band.on || band.gainDb == 0.0f) continue;
        const float fc = std::max(1.0f, band.freq);
        const float q  = std::max(0.1f, band.q);
        // Log-frequency Gaussian centred at fc, width ~ 1/q decades.
        const float logFc = std::log2(fc);
        const float sigma = 1.0f / (2.0f * q);
        for (int i = 1; i < numBins_; ++i)
        {
            const float f = static_cast<float>(i) * binWidth;
            const float dLog = std::log2(f) - logFc;
            const float shape = std::exp(-0.5f * (dLog * dLog) / (sigma * sigma));
            curve_[static_cast<size_t>(i)] += band.gainDb * shape;
        }
    }
}

} // namespace tonalizer
