#include "dsp/PitchDetector.h"
#include <cmath>
#include <algorithm>

namespace tonalizer {

void PitchDetector::prepare(double sampleRate)
{
    sampleRate_ = sampleRate;
    confidence_ = 0.0f;
}

void PitchDetector::reset()
{
    confidence_ = 0.0f;
}

float PitchDetector::detect(const float* samples, int windowSize)
{
    // YIN pitch detection algorithm
    // Half-window is the max lag we search
    int halfWindow = windowSize / 2;

    // Compute max and min lag from frequency range
    int minLag = std::max(1, static_cast<int>(sampleRate_ / kMaxFreq));
    int maxLag = std::min(halfWindow, static_cast<int>(sampleRate_ / kMinFreq));

    if (maxLag <= minLag || maxLag >= halfWindow)
    {
        confidence_ = 0.0f;
        return 0.0f;
    }

    // Resize working buffer
    yinBuffer_.resize(static_cast<size_t>(halfWindow));

    // Step 1: Difference function
    yinBuffer_[0] = 0.0f;
    for (int tau = 1; tau < halfWindow; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < halfWindow; ++j)
        {
            float diff = samples[j] - samples[j + tau];
            sum += diff * diff;
        }
        yinBuffer_[static_cast<size_t>(tau)] = sum;
    }

    // Step 2: Cumulative mean normalized difference
    yinBuffer_[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau < halfWindow; ++tau)
    {
        runningSum += yinBuffer_[static_cast<size_t>(tau)];
        if (runningSum > 0.0f)
            yinBuffer_[static_cast<size_t>(tau)] *= static_cast<float>(tau) / runningSum;
        else
            yinBuffer_[static_cast<size_t>(tau)] = 1.0f;
    }

    // Step 3: Absolute threshold — find first dip below threshold
    int bestTau = -1;
    for (int tau = minLag; tau < maxLag; ++tau)
    {
        if (yinBuffer_[static_cast<size_t>(tau)] < kThreshold)
        {
            // Find the local minimum
            while (tau + 1 < maxLag &&
                   yinBuffer_[static_cast<size_t>(tau + 1)] < yinBuffer_[static_cast<size_t>(tau)])
            {
                ++tau;
            }
            bestTau = tau;
            break;
        }
    }

    // If no dip below threshold, find global minimum in range
    if (bestTau < 0)
    {
        float minVal = 1.0f;
        bestTau = minLag;
        for (int tau = minLag; tau < maxLag; ++tau)
        {
            if (yinBuffer_[static_cast<size_t>(tau)] < minVal)
            {
                minVal = yinBuffer_[static_cast<size_t>(tau)];
                bestTau = tau;
            }
        }

        // If the global minimum is still too high, signal unvoiced
        if (minVal > 0.5f)
        {
            confidence_ = 0.0f;
            return 0.0f;
        }
    }

    // Step 4: Parabolic interpolation around bestTau
    float betterTau = static_cast<float>(bestTau);
    if (bestTau > 0 && bestTau < halfWindow - 1)
    {
        float s0 = yinBuffer_[static_cast<size_t>(bestTau - 1)];
        float s1 = yinBuffer_[static_cast<size_t>(bestTau)];
        float s2 = yinBuffer_[static_cast<size_t>(bestTau + 1)];

        float denom = 2.0f * s1 - s2 - s0;
        if (std::abs(denom) > 1e-10f)
        {
            betterTau = static_cast<float>(bestTau) + (s0 - s2) / (2.0f * denom);
        }
    }

    // Compute confidence: 1 - yinBuffer[bestTau]
    confidence_ = std::clamp(1.0f - yinBuffer_[static_cast<size_t>(bestTau)], 0.0f, 1.0f);

    // Convert lag to frequency
    if (betterTau <= 0.0f)
    {
        confidence_ = 0.0f;
        return 0.0f;
    }

    float freq = static_cast<float>(sampleRate_) / betterTau;

    // Final range check
    if (freq < kMinFreq || freq > kMaxFreq)
    {
        confidence_ = 0.0f;
        return 0.0f;
    }

    return freq;
}

} // namespace tonalizer
