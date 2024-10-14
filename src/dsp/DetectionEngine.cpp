#include "dsp/DetectionEngine.h"
#include <algorithm>
#include <cmath>

namespace tonalizer {

namespace {
constexpr float kEps = 1.0e-12f;
constexpr float kDbFloor = -120.0f;

inline float linToDb(float x) { return 20.0f * std::log10(std::max(kEps, x)); }
inline float dbToLin(float x) { return std::pow(10.0f, x * (1.0f / 20.0f)); }
} // namespace

void DetectionEngine::prepare(int numBins, double sampleRate, int hopSize)
{
    sampleRate_ = sampleRate;
    hopSize_ = std::max(1, hopSize);

    const size_t n = static_cast<size_t>(std::max(1, numBins));
    atonality_.assign(n, 0.0f);
    bandDb_.assign(n, 0.0f);
    peerMag_.assign(n, 0.0f);
    detectionMag_.assign(n, 0.0f);
    prefixSum_.assign(n + 1, 0.0f);
    envelope_.assign(n, 0.0f);
    targetGain_.assign(n, 1.0f);
    freqSmoothedGain_.assign(n, 1.0f);
    smoothedGain_.assign(n, 1.0f);
    hasPeer_ = false;

    // 1/3-octave envelope: for bin b (centre freq fc), half-width in bins
    // corresponds to fc * (2^(1/6) - 2^(-1/6)) / 2 divided by binWidth.
    // Clamp to a minimum of 1 bin to avoid degenerate env == mag for low bins.
    envHalfWidth_.assign(n, 1);
    const float binWidth = static_cast<float>(sampleRate_) /
                           static_cast<float>((numBins - 1) * 2);
    const float oneSixthOct = std::pow(2.0f, 1.0f / 6.0f);
    for (int b = 0; b < numBins; ++b)
    {
        const float fc = static_cast<float>(b) * binWidth;
        const float width = fc * (oneSixthOct - 1.0f / oneSixthOct);
        int bins = static_cast<int>(std::round(0.5f * width / std::max(binWidth, 1.0f)));
        bins = std::max(1, std::min(bins, numBins / 4));
        envHalfWidth_[static_cast<size_t>(b)] = bins;
    }

    recomputeCoeffs();
}

void DetectionEngine::reset()
{
    std::fill(smoothedGain_.begin(), smoothedGain_.end(), 1.0f);
    std::fill(freqSmoothedGain_.begin(), freqSmoothedGain_.end(), 1.0f);
    std::fill(targetGain_.begin(), targetGain_.end(), 1.0f);
    std::fill(peerMag_.begin(), peerMag_.end(), 0.0f);
    hasPeer_ = false;
}

void DetectionEngine::recomputeCoeffs()
{
    const float hopDur = static_cast<float>(hopSize_) / static_cast<float>(sampleRate_);
    const float atkTau = std::max(1.0e-4f, params_.attackMs  * 0.001f);
    const float relTau = std::max(1.0e-4f, params_.releaseMs * 0.001f);
    attackCoeff_  = 1.0f - std::exp(-hopDur / atkTau);
    releaseCoeff_ = 1.0f - std::exp(-hopDur / relTau);
}

void DetectionEngine::setParams(const Params& p)
{
    const bool timingChanged =
        p.attackMs != params_.attackMs || p.releaseMs != params_.releaseMs;
    params_ = p;
    params_.depth       = std::clamp(params_.depth, 0.0f, 1.0f);
    params_.ratio       = std::max(1.0f, params_.ratio);
    params_.kneeDb      = std::max(0.0f, params_.kneeDb);
    params_.selectivity = std::clamp(params_.selectivity, 0.0f, 1.0f);
    params_.softHard    = std::clamp(params_.softHard, 0.0f, 1.0f);
    params_.crossfeed   = std::clamp(params_.crossfeed, 0.0f, 1.0f);
    if (timingChanged) recomputeCoeffs();
}

void DetectionEngine::setAtonalityBias(const float* atonality, int numBins)
{
    if (atonality == nullptr || numBins <= 0) return;
    const int n = std::min(numBins, static_cast<int>(atonality_.size()));
    for (int i = 0; i < n; ++i)
        atonality_[static_cast<size_t>(i)] = std::clamp(atonality[i], 0.0f, 1.0f);
}

void DetectionEngine::setBandCurve(const float* bandDbCurve, int numBins)
{
    if (bandDbCurve == nullptr || numBins <= 0) { std::fill(bandDb_.begin(), bandDb_.end(), 0.0f); return; }
    const int n = std::min(numBins, static_cast<int>(bandDb_.size()));
    for (int i = 0; i < n; ++i)
        bandDb_[static_cast<size_t>(i)] = bandDbCurve[i];
}

void DetectionEngine::setPeerMagnitude(const float* peerMag, int numBins)
{
    if (peerMag == nullptr || numBins <= 0) { hasPeer_ = false; return; }
    const int n = std::min(numBins, static_cast<int>(peerMag_.size()));
    for (int i = 0; i < n; ++i)
        peerMag_[static_cast<size_t>(i)] = peerMag[i];
    hasPeer_ = true;
}

void DetectionEngine::process(const float* inputMag, float* outGains, int numBins)
{
    if (inputMag == nullptr || outGains == nullptr) return;
    const int n = std::min(numBins, static_cast<int>(detectionMag_.size()));
    if (n <= 0) return;

    // Stage 1 - assemble detection magnitude (self + optional peer crossfeed).
    const float cf = hasPeer_ ? params_.crossfeed : 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float s = inputMag[i];
        const float p = hasPeer_ ? peerMag_[static_cast<size_t>(i)] : s;
        detectionMag_[static_cast<size_t>(i)] = (1.0f - cf) * s + cf * std::max(s, p);
    }

    // Stage 2 - spectral envelope via log-freq box mean on detection magnitude.
    // Per-bin window widths vary (1/3-octave), so we use a prefix sum for O(N)
    // total cost regardless of window size — required for the Insane FFT tier
    // where the top bins span thousands of samples.
    prefixSum_[0] = 0.0f;
    for (int i = 0; i < n; ++i)
        prefixSum_[static_cast<size_t>(i + 1)] =
            prefixSum_[static_cast<size_t>(i)] + detectionMag_[static_cast<size_t>(i)];

    for (int i = 0; i < n; ++i)
    {
        const int w = envHalfWidth_[static_cast<size_t>(i)];
        const int lo = std::max(0, i - w);
        const int hi = std::min(n - 1, i + w);
        const float sum = prefixSum_[static_cast<size_t>(hi + 1)] -
                          prefixSum_[static_cast<size_t>(lo)];
        envelope_[static_cast<size_t>(i)] = sum / static_cast<float>(hi - lo + 1);
    }

    // Stage 3 - scale-first salience. atonality_[bin] is 0 for in-scale bins
    // (always protected) and 1 for fully out-of-scale bins. selectivity gates
    // whether an atonal bin must also stick above the spectral envelope by
    // threshold dB before we touch it.
    //   sel=0 -> pure scale-lock (any atonal content gets reduced).
    //   sel=1 -> atonal AND peaky (leaves low-level out-of-key content alone).
    //
    // softHard widens the knee of the envelope gate: hard = sharp threshold,
    // soft = smooth transition over a larger dB range.
    // Bands (bandDb_) apply a per-region multiplier to the atonal mask: +dB
    // hunts harder in that region; sufficiently negative dB protects it.
    const float thr = params_.thresholdDb;
    const float knee = params_.kneeDb + 12.0f * (1.0f - params_.softHard);
    const float sel = params_.selectivity;
    // Compressor-style ratio: 1.0 -> no reduction at all, large -> full 60 dB.
    // Acts as a global ceiling on top of depth, so the user gets a familiar
    // "gentle vs surgical" knob even though the per-bin gating is already
    // scale-driven.
    const float ratioScale = 1.0f - 1.0f / std::max(1.0f, params_.ratio);
    const float maxReductionDb = 60.0f * ratioScale;

    for (int i = 0; i < n; ++i)
    {
        float atonalMask = atonality_[static_cast<size_t>(i)];

        // Band overlay as a per-bin atonal-mask multiplier. +6 dB -> ~x2 weight,
        // -inf protects the region entirely.
        const float bandScale = dbToLin(bandDb_[static_cast<size_t>(i)]);
        atonalMask = std::clamp(atonalMask * bandScale, 0.0f, 1.0f);

        // Envelope salience gate (0..1). Smoothstep over [thr - knee/2, thr + knee/2].
        float envelopeGate = 1.0f;
        if (sel > 0.0f)
        {
            const float salienceDb = linToDb(detectionMag_[static_cast<size_t>(i)]) -
                                     linToDb(envelope_[static_cast<size_t>(i)]);
            const float kneeHalf = std::max(1.0e-3f, 0.5f * knee);
            const float t = std::clamp((salienceDb - (thr - kneeHalf)) /
                                        (2.0f * kneeHalf), 0.0f, 1.0f);
            const float smooth = t * t * (3.0f - 2.0f * t);
            envelopeGate = smooth;
        }

        // Selectivity blends: sel=0 means envelope irrelevant; sel=1 gates
        // fully on envelope.
        const float mask = atonalMask * ((1.0f - sel) + sel * envelopeGate);
        const float reductionDb = mask * params_.depth * maxReductionDb;
        targetGain_[static_cast<size_t>(i)] = dbToLin(-reductionDb);
    }

    // Stage 4 - light frequency smoothing to kill isolated-bin musical noise.
    if (n == 1)
    {
        freqSmoothedGain_[0] = targetGain_[0];
    }
    else
    {
        freqSmoothedGain_[0] = 0.75f * targetGain_[0] + 0.25f * targetGain_[1];
        for (int i = 1; i < n - 1; ++i)
        {
            freqSmoothedGain_[static_cast<size_t>(i)] =
                0.25f * targetGain_[static_cast<size_t>(i - 1)] +
                0.50f * targetGain_[static_cast<size_t>(i)] +
                0.25f * targetGain_[static_cast<size_t>(i + 1)];
        }
        freqSmoothedGain_[static_cast<size_t>(n - 1)] =
            0.25f * targetGain_[static_cast<size_t>(n - 2)] +
            0.75f * targetGain_[static_cast<size_t>(n - 1)];
    }

    // Stage 5 - per-bin attack/release.
    for (int i = 0; i < n; ++i)
    {
        const float target = freqSmoothedGain_[static_cast<size_t>(i)];
        float& current = smoothedGain_[static_cast<size_t>(i)];
        const float coeff = (target < current) ? attackCoeff_ : releaseCoeff_;
        current += coeff * (target - current);
        outGains[i] = current;
    }
}

} // namespace tonalizer
