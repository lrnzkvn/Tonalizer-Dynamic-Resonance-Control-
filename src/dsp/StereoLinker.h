#pragma once
#include <array>
#include <vector>
#include <algorithm>

namespace tonalizer {

// Tiny shared scratchpad used by per-channel DetectionEngines to read each
// other's detection magnitudes. Audio-thread only; per-channel STFT hops
// run sequentially under the current architecture so no locking is needed.
class StereoLinker
{
public:
    static constexpr int kMaxChannels = 2;

    void prepare(int numBins)
    {
        numBins_ = numBins;
        for (auto& buf : mags_) buf.assign(static_cast<size_t>(numBins), 0.0f);
        for (auto& v : valid_) v = false;
    }

    void publish(int channel, const float* mag, int numBins)
    {
        if (channel < 0 || channel >= kMaxChannels || mag == nullptr) return;
        const int n = std::min(numBins, numBins_);
        auto& dst = mags_[static_cast<size_t>(channel)];
        for (int i = 0; i < n; ++i) dst[static_cast<size_t>(i)] = mag[i];
        valid_[static_cast<size_t>(channel)] = true;
    }

    // Returns peer magnitude buffer or nullptr if peer hasn't posted yet.
    const float* peer(int channel) const
    {
        const int other = 1 - channel;
        if (channel < 0 || channel >= kMaxChannels) return nullptr;
        if (! valid_[static_cast<size_t>(other)]) return nullptr;
        return mags_[static_cast<size_t>(other)].data();
    }

    int getNumBins() const { return numBins_; }

private:
    std::array<std::vector<float>, kMaxChannels> mags_;
    std::array<bool, kMaxChannels> valid_ { false, false };
    int numBins_ = 0;
};

} // namespace tonalizer
