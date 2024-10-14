#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

namespace tonalizer {

namespace {
const juce::Identifier kEditorWidthProperty { "editorWidth" };
const juce::Identifier kEditorHeightProperty { "editorHeight" };

template <typename T>
T* findParam(juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    return dynamic_cast<T*>(apvts.getParameter(id));
}
} // namespace

TonalizerProcessor::TonalizerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput ("Input",     juce::AudioChannelSet::stereo(), true)
                         .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)
                         .withOutput("Output",    juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "TonalizerState", createParameterLayout())
{
    rootNoteParam         = findParam<juce::AudioParameterChoice>(apvts, ParamIDs::rootNote);
    scaleTypeParam        = findParam<juce::AudioParameterChoice>(apvts, ParamIDs::scaleType);
    eraseAmountParam      = findParam<juce::AudioParameterFloat>(apvts,  ParamIDs::eraseAmount);
    sharpnessParam        = findParam<juce::AudioParameterFloat>(apvts,  ParamIDs::eraseSharpness);
    dryWetParam           = findParam<juce::AudioParameterFloat>(apvts,  ParamIDs::dryWet);
    crossfadeParam        = findParam<juce::AudioParameterFloat>(apvts,  ParamIDs::crossfade);
    retuneSpeedParam      = findParam<juce::AudioParameterFloat>(apvts,  ParamIDs::retuneSpeed);
    correctionAmountParam = findParam<juce::AudioParameterFloat>(apvts,  ParamIDs::correctionAmount);
    bypassParam           = findParam<juce::AudioParameterBool>(apvts,   ParamIDs::bypass);
    deltaParam            = findParam<juce::AudioParameterBool>(apvts,   ParamIDs::delta);

    depthParam       = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::depth);
    thresholdParam   = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::threshold);
    ratioParam       = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::ratio);
    kneeParam        = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::knee);
    selectivityParam = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::selectivity);
    softHardParam    = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::softHard);
    attackParam      = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::attackMs);
    releaseParam     = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::releaseMs);
    crossfeedParam   = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::crossfeed);
    stereoLinkParam  = findParam<juce::AudioParameterBool>(apvts,  ParamIDs::stereoLink);
    stereoModeParam  = findParam<juce::AudioParameterChoice>(apvts, ParamIDs::stereoMode);
    sidechainOnParam = findParam<juce::AudioParameterBool>(apvts,  ParamIDs::sidechainOn);
    qualityParam        = findParam<juce::AudioParameterChoice>(apvts, ParamIDs::quality);
    offlineQualityParam = findParam<juce::AudioParameterChoice>(apvts, ParamIDs::offlineQuality);

    for (int i = 0; i < BandOverlay::kNumBands; ++i)
    {
        bandParams_[static_cast<size_t>(i)].freq = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::bandFreq(i));
        bandParams_[static_cast<size_t>(i)].q    = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::bandQ(i));
        bandParams_[static_cast<size_t>(i)].gain = findParam<juce::AudioParameterFloat>(apvts, ParamIDs::bandGain(i));
        bandParams_[static_cast<size_t>(i)].on   = findParam<juce::AudioParameterBool>(apvts,  ParamIDs::bandOn(i));
    }

    // Quality changes are handled asynchronously on the message thread with
    // processing suspended - keeps audio-thread allocations out of the hot path.
    apvts.addParameterListener(ParamIDs::quality,        this);
    apvts.addParameterListener(ParamIDs::offlineQuality, this);
}

TonalizerProcessor::~TonalizerProcessor()
{
    apvts.removeParameterListener(ParamIDs::quality,        this);
    apvts.removeParameterListener(ParamIDs::offlineQuality, this);
}

void TonalizerProcessor::parameterChanged(const juce::String& id, float /*newValue*/)
{
    if (id == ParamIDs::quality || id == ParamIDs::offlineQuality)
    {
        // Only the currently-active tier drives a rebuild. Editing the idle
        // tier stores its value silently; it takes effect when the next
        // offline/realtime transition occurs.
        const bool offline = lastOfflineFlag_.load();
        const bool relevant = (offline && id == ParamIDs::offlineQuality)
                           || (! offline && id == ParamIDs::quality);
        if (! relevant) return;

        const int idx = offline ? offlineQualityParam->getIndex()
                                : qualityParam->getIndex();
        pendingFFTOrder_.store(qualityIndexToFFTOrder(idx));
        triggerAsyncUpdate();
    }
}

void TonalizerProcessor::handleAsyncUpdate()
{
    const int order = pendingFFTOrder_.exchange(-1);
    if (order < 0 || order == lastFFTOrder_) return;

    // Suspend so processBlock() can't race the rebuild.
    suspendProcessing(true);
    applyQuality(order);
    suspendProcessing(false);
}

int TonalizerProcessor::qualityIndexToFFTOrder(int idx) const
{
    // 0..4 -> orders 10..14 (1024..16384). 75% overlap => hop = FFT/4.
    static constexpr int kOrders[5] = { 10, 11, 12, 13, 14 };
    return kOrders[std::clamp(idx, 0, 4)];
}

void TonalizerProcessor::applyQuality(int fftOrder)
{
    if (fftOrder == lastFFTOrder_) return;

    for (auto& morpher : morphers_)
        morpher.prepare(currentSampleRate_, lastMaxBlockSize_, fftOrder);

    const int numBins = morphers_[0].getNumBins();
    const int hop     = morphers_[0].getHopSize();

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        erasers_[static_cast<size_t>(ch)].prepare(numBins, currentSampleRate_, hop);
        erasers_[static_cast<size_t>(ch)].setChannelIndex(ch);
        erasers_[static_cast<size_t>(ch)].setStereoLinker(&linker_);
        erasers_[static_cast<size_t>(ch)].setBandOverlay(&bands_);
        morphers_[static_cast<size_t>(ch)].setEraser(&erasers_[static_cast<size_t>(ch)]);
    }

    linker_.prepare(numBins);
    bands_.prepare(numBins, currentSampleRate_);
    for (auto& s : lastBandState_) s = {};
    updateBandsIfNeeded();

    // TonalClassifier precomputes atonality against the new FFT size.
    lastRootNote_ = -1;
    lastScaleType_ = -1;
    updateScaleIfNeeded();

    setLatencySamples(morphers_[0].getFFTSize());
    lastFFTOrder_ = fftOrder;
}

void TonalizerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate_ = sampleRate;
    lastMaxBlockSize_ = samplesPerBlock;
    lastFFTOrder_ = -1; // force full rebuild on first call to applyQuality

    const int liveIdx = qualityParam != nullptr ? qualityParam->getIndex() : 1;
    applyQuality(qualityIndexToFFTOrder(liveIdx));

    dryBuffer_.setSize(getTotalNumInputChannels(), samplesPerBlock);

    // 30 ms ramp on knob-prone detection params. Snap to the current value so
    // the first block doesn't slide up from 0.
    const double rampSec = 0.030;
    smDepth_      .reset(sampleRate, rampSec);
    smThresholdDb_.reset(sampleRate, rampSec);
    smSelectivity_.reset(sampleRate, rampSec);
    if (depthParam)       smDepth_      .setCurrentAndTargetValue(depthParam->get()       / 100.0f);
    if (thresholdParam)   smThresholdDb_.setCurrentAndTargetValue(thresholdParam->get());
    if (selectivityParam) smSelectivity_.setCurrentAndTargetValue(selectivityParam->get() / 100.0f);
}

void TonalizerProcessor::releaseResources()
{
    for (auto& morpher : morphers_) morpher.reset();
    for (auto& eraser  : erasers_)  eraser.reset();
}

bool TonalizerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();
    if (mainOut != mainIn) return false;
    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo()) return false;

    // Sidechain (if present) must be mono or stereo.
    if (layouts.inputBuses.size() > 1)
    {
        const auto& sc = layouts.inputBuses[1];
        if (! sc.isDisabled()
            && sc != juce::AudioChannelSet::mono()
            && sc != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}

void TonalizerProcessor::pushDetectionParams()
{
    // Re-target the smoothers from APVTS, advance by one block worth of samples,
    // then sample the smoothed values. This kills clicks on fast knob moves.
    smDepth_      .setTargetValue(depthParam->get()       / 100.0f);
    smThresholdDb_.setTargetValue(thresholdParam->get());
    smSelectivity_.setTargetValue(selectivityParam->get() / 100.0f);

    const int blk = std::max(1, lastMaxBlockSize_);
    smDepth_      .skip(blk);
    smThresholdDb_.skip(blk);
    smSelectivity_.skip(blk);

    DetectionEngine::Params p;
    p.depth       = smDepth_.getCurrentValue();
    p.thresholdDb = smThresholdDb_.getCurrentValue();
    p.ratio       = ratioParam->get();
    p.kneeDb      = kneeParam->get();
    p.selectivity = smSelectivity_.getCurrentValue();
    p.softHard    = softHardParam->get()    / 100.0f;
    p.attackMs    = attackParam->get();
    p.releaseMs   = releaseParam->get();
    p.crossfeed   = stereoLinkParam->get() ? (crossfeedParam->get() / 100.0f) : 0.0f;
    for (auto& e : erasers_) e.setDetectionParams(p);
}

void TonalizerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalIn  = getTotalNumInputChannels();
    const auto totalOut = getTotalNumOutputChannels();
    const int  numSamples = buffer.getNumSamples();

    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear(i, 0, numSamples);

    if (bypassParam->get()) return;

    // Detect realtime <-> offline transition and defer the quality swap to the
    // message thread. applyQuality on the audio thread would reallocate buffers.
    const bool offline = isNonRealtime();
    if (offline != lastOfflineFlag_.load())
    {
        lastOfflineFlag_.store(offline);
        const int liveIdx    = qualityParam != nullptr ? qualityParam->getIndex() : 1;
        const int offlineIdx = offlineQualityParam != nullptr ? offlineQualityParam->getIndex() : liveIdx;
        pendingFFTOrder_.store(qualityIndexToFFTOrder(offline ? offlineIdx : liveIdx));
        triggerAsyncUpdate();
        // Don't try to process this block - the async swap will take effect
        // before the next one. suspendProcessing() hasn't fired yet so just
        // short-circuit to silence-through-dry.
        return;
    }

    updateScaleIfNeeded();
    updateBandsIfNeeded();
    pushDetectionParams();

    // Legacy surface still drives the detector when the user hasn't touched
    // the new knobs - keeps existing editor responsive.
    for (auto& e : erasers_)
        e.setParameters(eraseAmountParam->get(), sharpnessParam->get());
    pushDetectionParams(); // new params win over legacy mapping

    const float crossfade = crossfadeParam->get() / 100.0f;
    const float dryWet    = dryWetParam->get()    / 100.0f;
    const float retuneMs  = retuneSpeedParam->get();
    const float correction= correctionAmountParam->get() / 100.0f;

    for (auto& m : morphers_)
    {
        m.setRetuneSpeed(retuneMs);
        m.setCorrectionAmount(correction);
        m.setSpectralCrossfade(crossfade);
    }

    const int numChannels = std::min(totalIn, kMaxChannels);

    dryBuffer_.makeCopyOf(buffer, true);

    // Sidechain bus resolution.
    const bool scUserOn = sidechainOnParam != nullptr && sidechainOnParam->get();
    juce::AudioBuffer<float> scBus = (getBusCount(true) > 1)
        ? getBusBuffer(buffer, true, 1)
        : juce::AudioBuffer<float>();
    const bool scActive = scUserOn && scBus.getNumChannels() > 0;

    for (auto& m : morphers_) m.setSidechainEnabled(scActive);

    // Stereo mode: when MS and we have two channels, encode time-domain
    // L/R -> M/S before STFT so channel 0 = Mid, channel 1 = Side. Decode
    // after processing. Pitch detection always runs on ch0 (= Mid in MS).
    const bool useMS = (numChannels >= 2)
                        && stereoModeParam != nullptr
                        && stereoModeParam->getIndex() == 1;
    constexpr float kMSNorm = 0.70710678f; // 1/sqrt(2) for power-preserving M/S

    float* L = buffer.getWritePointer(0);
    float* R = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
    const float* scL = scActive ? scBus.getReadPointer(0) : nullptr;
    const float* scR = (scActive && scBus.getNumChannels() > 1)
                        ? scBus.getReadPointer(1) : scL;

    morphers_[0].setPitchDetectEnabled(true);
    if (numChannels > 1)
        morphers_[1].setPitchDetectEnabled(false);

    // Interleaved per-sample loop: both channels advance one sample per
    // iteration so each hop triggered by ch0 is immediately followed by ch1's
    // hop (same audio frame), giving same-block stereo linking with zero lag.
    if (numChannels == 1)
    {
        for (int i = 0; i < numSamples; ++i)
            L[i] = morphers_[0].processSample(L[i], scActive ? scL[i] : 0.0f, scActive);
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float a = L[i];
            float b = R[i];
            float sa = scActive ? scL[i] : 0.0f;
            float sb = scActive ? scR[i] : 0.0f;

            if (useMS)
            {
                const float m = (a + b) * kMSNorm;
                const float s = (a - b) * kMSNorm;
                a = m; b = s;
                const float sm = (sa + sb) * kMSNorm;
                const float ss = (sa - sb) * kMSNorm;
                sa = sm; sb = ss;
            }

            a = morphers_[0].processSample(a, sa, scActive);
            b = morphers_[1].processSample(b, sb, scActive);

            if (useMS)
            {
                // Decode M/S -> L/R with the same normalization so unity
                // M/S -> L/R gain is preserved.
                const float ll = (a + b) * kMSNorm;
                const float rr = (a - b) * kMSNorm;
                L[i] = ll;
                R[i] = rr;
            }
            else
            {
                L[i] = a;
                R[i] = b;
            }
        }

        // Coherent pitch-shift ratio still shared channel-to-channel in LR
        // mode. In MS mode, Mid/Side are independent and the Side channel
        // gets its own ratio (detector is disabled there so it stays at 1.0).
        if (! useMS)
            morphers_[1].setSmoothedRatio(morphers_[0].getSmoothedRatio());
    }

    // Delta mode: output removed spectrum only (wet - dry). Else dry/wet mix.
    const bool delta = deltaParam->get();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* wet = buffer.getWritePointer(ch);
        const float* dry = dryBuffer_.getReadPointer(ch);
        if (delta)
        {
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] - wet[i];
        }
        else if (dryWet < 1.0f)
        {
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] * (1.0f - dryWet) + wet[i] * dryWet;
        }
    }

    {
        std::unique_lock<std::mutex> lock(vizMutex_, std::try_to_lock);
        if (lock.owns_lock())
        {
            const float* gains = morphers_[0].getEraseGains();
            const float* mags  = morphers_[0].getMagnitudes();
            const float* aton  = classifier_.getAtonalityData();
            const int nb = std::min(morphers_[0].getNumBins(),
                                    static_cast<int>(vizData_.gains.size()));
            for (int i = 0; i < nb; ++i)
            {
                vizData_.magnitudes[static_cast<size_t>(i)] = mags[i];
                vizData_.gains[static_cast<size_t>(i)] = gains[i];
                vizData_.atonality[static_cast<size_t>(i)] = aton[i];
            }
            if (numChannels > 1)
            {
                const float* mags1 = morphers_[1].getMagnitudes();
                for (int i = 0; i < nb; ++i)
                    vizData_.magnitudesCh1[static_cast<size_t>(i)] = mags1[i];
            }
            else
            {
                for (int i = 0; i < nb; ++i)
                    vizData_.magnitudesCh1[static_cast<size_t>(i)] = 0.0f;
            }
            vizData_.numBins = nb;
            vizData_.fftSize = morphers_[0].getFFTSize();
            vizData_.numChannels = numChannels;
            vizData_.msMode = useMS;
            vizData_.ready = true;
        }
    }
}

void TonalizerProcessor::updateScaleIfNeeded()
{
    const int root = rootNoteParam->getIndex();
    const int scale = scaleTypeParam->getIndex();
    if (root == lastRootNote_ && scale == lastScaleType_) return;

    lastRootNote_ = root;
    lastScaleType_ = scale;
    currentScale_ = MusicalScale(root, static_cast<MusicalScale::ScaleType>(scale));

    for (auto& m : morphers_) m.setScale(currentScale_);
    classifier_.prepare(currentScale_, currentSampleRate_, morphers_[0].getFFTSize());
    for (auto& e : erasers_)
        e.setAtonality(classifier_.getAtonalityData(), classifier_.getNumBins());
}

void TonalizerProcessor::updateBandsIfNeeded()
{
    bool dirty = false;
    for (int i = 0; i < BandOverlay::kNumBands; ++i)
    {
        const auto& pp = bandParams_[static_cast<size_t>(i)];
        if (pp.freq == nullptr) continue;
        BandOverlay::Band nb;
        nb.freq   = pp.freq->get();
        nb.q      = pp.q->get();
        nb.gainDb = pp.gain->get();
        nb.on     = pp.on->get();
        auto& last = lastBandState_[static_cast<size_t>(i)];
        if (nb.freq != last.freq || nb.q != last.q
            || nb.gainDb != last.gainDb || nb.on != last.on)
        {
            bands_.setBand(i, nb);
            last = nb;
            dirty = true;
        }
    }
    if (dirty) bands_.recompute();
}

TonalizerProcessor::VisualizationData TonalizerProcessor::getVisualizationSnapshot() const
{
    std::lock_guard<std::mutex> lock(vizMutex_);
    return vizData_;
}

SpectralMorpher::PitchInfo TonalizerProcessor::getAutotunePitchInfo() const
{
    return morphers_[0].getPitchInfo();
}

juce::Point<int> TonalizerProcessor::getStoredEditorSize() const
{
    return {
        static_cast<int>(apvts.state.getProperty(kEditorWidthProperty, 0)),
        static_cast<int>(apvts.state.getProperty(kEditorHeightProperty, 0))
    };
}

void TonalizerProcessor::setStoredEditorSize(juce::Point<int> size)
{
    auto& state = apvts.state;
    if (static_cast<int>(state.getProperty(kEditorWidthProperty, 0)) == size.x
        && static_cast<int>(state.getProperty(kEditorHeightProperty, 0)) == size.y)
        return;
    state.setProperty(kEditorWidthProperty, size.x, nullptr);
    state.setProperty(kEditorHeightProperty, size.y, nullptr);
}

juce::AudioProcessorEditor* TonalizerProcessor::createEditor() { return new TonalizerEditor(*this); }

void TonalizerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TonalizerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

} // namespace tonalizer

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new tonalizer::TonalizerProcessor();
}
