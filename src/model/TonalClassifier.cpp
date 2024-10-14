#include "model/TonalClassifier.h"
#include <cmath>

namespace tonalizer {

void TonalClassifier::prepare(const MusicalScale& scale,
                              double sampleRate,
                              int fftSize)
{
    const int numBins = fftSize / 2 + 1;
    atonality_.resize(static_cast<size_t>(numBins));

    const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(fftSize);

    for (int i = 0; i < numBins; ++i)
    {
        const float freq = static_cast<float>(i) * binWidth;
        atonality_[static_cast<size_t>(i)] = scale.tonalDistanceForFreq(freq);
    }
}

float TonalClassifier::getAtonality(int binIndex) const
{
    if (binIndex < 0 || binIndex >= static_cast<int>(atonality_.size()))
        return 0.0f;
    return atonality_[static_cast<size_t>(binIndex)];
}

} // namespace tonalizer
