#pragma once
#include "model/MusicalScale.h"
#include <vector>

namespace tonalizer {

class TonalClassifier
{
public:
    TonalClassifier() = default;

    // Recompute per-bin atonality scores based on scale.
    void prepare(const MusicalScale& scale,
                 double sampleRate,
                 int fftSize);

    // Get the precomputed atonality score for a given bin index.
    float getAtonality(int binIndex) const;

    // Get pointer to the full atonality array (numBins = fftSize/2 + 1).
    const float* getAtonalityData() const { return atonality_.data(); }

    int getNumBins() const { return static_cast<int>(atonality_.size()); }

private:
    std::vector<float> atonality_;
};

} // namespace tonalizer
