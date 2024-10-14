#include "dsp/SpectralMorpher.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace tonalizer {

SpectralMorpher::SpectralMorpher() = default;
SpectralMorpher::~SpectralMorpher() = default;

void SpectralMorpher::prepare(double sampleRate, int maxBlockSize)
{
    prepare(sampleRate, maxBlockSize, fftOrder_);
}

void SpectralMorpher::prepare(double sampleRate, int /*maxBlockSize*/, int fftOrder)
{
    sampleRate_ = sampleRate;
    fftOrder_ = std::clamp(fftOrder, 6, kMaxFFTOrder); // 64 .. 16384
    fftSize_ = 1 << fftOrder_;
    hopSize_ = fftSize_ / 4;
    numBins_ = fftSize_ / 2 + 1;

    fft_   = std::make_unique<juce::dsp::FFT>(fftOrder_);
    scFft_ = std::make_unique<juce::dsp::FFT>(fftOrder_);

    window_.resize(static_cast<size_t>(fftSize_));
    for (int i = 0; i < fftSize_; ++i)
    {
        float x = static_cast<float>(i) / static_cast<float>(fftSize_);
        window_[static_cast<size_t>(i)] = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * x);
    }

    const size_t ring = static_cast<size_t>(fftSize_ * 2);
    inputBuffer_.assign(ring, 0.0f);  inputWritePos_ = 0;
    outputBuffer_.assign(ring, 0.0f); outputReadPos_ = 0;
    scBuffer_.assign(ring, 0.0f);     scWritePos_ = 0;
    scHasData_ = false;
    hopCounter_ = 0;

    fftData_.assign(static_cast<size_t>(fftSize_ * 2), 0.0f);
    magnitudes_.assign(static_cast<size_t>(numBins_), 0.0f);
    phases_.assign(static_cast<size_t>(numBins_), 0.0f);

    scFftData_.assign(static_cast<size_t>(fftSize_ * 2), 0.0f);
    scMagnitudes_.assign(static_cast<size_t>(numBins_), 0.0f);

    eraseMags_.assign(static_cast<size_t>(numBins_), 0.0f);
    erasePhases_.assign(static_cast<size_t>(numBins_), 0.0f);

    prevPhases_.assign(static_cast<size_t>(numBins_), 0.0f);
    synthPhases_.assign(static_cast<size_t>(numBins_), 0.0f);
    analysisFreqs_.assign(static_cast<size_t>(numBins_), 0.0f);
    tuneMags_.assign(static_cast<size_t>(numBins_), 0.0f);
    tunePhases_.assign(static_cast<size_t>(numBins_), 0.0f);
    shiftedFreqs_.assign(static_cast<size_t>(numBins_), 0.0f);

    blendedMags_.assign(static_cast<size_t>(numBins_), 0.0f);
    blendedPhases_.assign(static_cast<size_t>(numBins_), 0.0f);

    pitchDetectBuffer_.assign(static_cast<size_t>(fftSize_), 0.0f);
    pitchDetector_.prepare(sampleRate);

    smoothedRatio_ = 1.0f;
    eraseGainsCache_.assign(static_cast<size_t>(numBins_), 1.0f);
}

void SpectralMorpher::reset()
{
    std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
    std::fill(outputBuffer_.begin(), outputBuffer_.end(), 0.0f);
    std::fill(scBuffer_.begin(), scBuffer_.end(), 0.0f);
    std::fill(prevPhases_.begin(), prevPhases_.end(), 0.0f);
    std::fill(synthPhases_.begin(), synthPhases_.end(), 0.0f);
    inputWritePos_ = 0;
    outputReadPos_ = 0;
    scWritePos_ = 0;
    scHasData_ = false;
    hopCounter_ = 0;
    smoothedRatio_ = 1.0f;
    pitchDetector_.reset();
}

void SpectralMorpher::setEraser(SpectralEraser* eraser) { eraser_ = eraser; }
void SpectralMorpher::setScale(const MusicalScale& scale) { scale_ = scale; }
void SpectralMorpher::setRetuneSpeed(float ms) { retuneSpeedMs_ = ms; }
void SpectralMorpher::setCorrectionAmount(float amount) { correctionAmount_ = amount; }
void SpectralMorpher::setSpectralCrossfade(float amount01) { spectralCrossfade_ = amount01; }
void SpectralMorpher::setSidechainEnabled(bool enabled) { sidechainEnabled_ = enabled; }
void SpectralMorpher::setPitchDetectEnabled(bool enabled) { pitchDetectEnabled_ = enabled; }
void SpectralMorpher::setSmoothedRatio(float ratio) { smoothedRatio_ = ratio; }
float SpectralMorpher::getSmoothedRatio() const { return smoothedRatio_; }

SpectralMorpher::PitchInfo SpectralMorpher::getPitchInfo() const
{
    PitchInfo info;
    info.detectedHz = pitchDetectedHz_.load(std::memory_order_relaxed);
    info.detectedSemitone = pitchDetectedSemi_.load(std::memory_order_relaxed);
    info.targetSemitone = pitchTargetSemi_.load(std::memory_order_relaxed);
    info.correctedSemitone = pitchCorrectedSemi_.load(std::memory_order_relaxed);
    info.confidence = pitchConfidence_.load(std::memory_order_relaxed);
    return info;
}

const float* SpectralMorpher::getEraseGains() const { return eraseGainsCache_.data(); }

void SpectralMorpher::processBlock(float* mainData, int numSamples)
{
    processBlock(mainData, nullptr, numSamples);
}

void SpectralMorpher::processBlock(float* mainData, const float* sidechainData, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        const float sc = (sidechainData != nullptr) ? sidechainData[i] : 0.0f;
        mainData[i] = processSample(mainData[i], sc, sidechainData != nullptr);
    }
}

float SpectralMorpher::processSample(float inSample, float sidechainSample, bool sidechainValid)
{
    const int ringSize = static_cast<int>(inputBuffer_.size());

    inputBuffer_[static_cast<size_t>(inputWritePos_)] = inSample;
    inputWritePos_ = (inputWritePos_ + 1) % ringSize;

    if (sidechainValid)
    {
        scBuffer_[static_cast<size_t>(scWritePos_)] = sidechainSample;
        scHasData_ = true;
    }
    else
    {
        scBuffer_[static_cast<size_t>(scWritePos_)] = 0.0f;
    }
    scWritePos_ = (scWritePos_ + 1) % ringSize;

    ++hopCounter_;
    if (hopCounter_ >= hopSize_)
    {
        hopCounter_ = 0;
        processMainFrame();
    }

    const float out = outputBuffer_[static_cast<size_t>(outputReadPos_)];
    outputBuffer_[static_cast<size_t>(outputReadPos_)] = 0.0f;
    outputReadPos_ = (outputReadPos_ + 1) % ringSize;
    return out;
}

void SpectralMorpher::processMainFrame()
{
    if (fft_ == nullptr) return;
    const int ringSize = static_cast<int>(inputBuffer_.size());
    const float twoPi = juce::MathConstants<float>::twoPi;

    int readPos = ((inputWritePos_ - fftSize_) % ringSize + ringSize) % ringSize;

    for (int i = 0; i < fftSize_; ++i)
    {
        int idx = (readPos + i) % ringSize;
        pitchDetectBuffer_[static_cast<size_t>(i)] = inputBuffer_[static_cast<size_t>(idx)];
    }

    for (int i = 0; i < fftSize_; ++i)
    {
        int idx = (readPos + i) % ringSize;
        fftData_[static_cast<size_t>(i)] = inputBuffer_[static_cast<size_t>(idx)]
                                            * window_[static_cast<size_t>(i)];
    }
    std::fill(fftData_.begin() + fftSize_, fftData_.end(), 0.0f);

    fft_->performRealOnlyForwardTransform(fftData_.data(), true);

    for (int bin = 0; bin < numBins_; ++bin)
    {
        float re = fftData_[static_cast<size_t>(bin * 2)];
        float im = fftData_[static_cast<size_t>(bin * 2 + 1)];
        magnitudes_[static_cast<size_t>(bin)] = std::sqrt(re * re + im * im);
        phases_[static_cast<size_t>(bin)] = std::atan2(im, re);
    }

    const bool useSidechain = sidechainEnabled_ && scHasData_;
    if (useSidechain)
    {
        int scRead = ((scWritePos_ - fftSize_) % ringSize + ringSize) % ringSize;
        for (int i = 0; i < fftSize_; ++i)
        {
            int idx = (scRead + i) % ringSize;
            scFftData_[static_cast<size_t>(i)] = scBuffer_[static_cast<size_t>(idx)]
                                                  * window_[static_cast<size_t>(i)];
        }
        std::fill(scFftData_.begin() + fftSize_, scFftData_.end(), 0.0f);
        scFft_->performRealOnlyForwardTransform(scFftData_.data(), true);
        for (int bin = 0; bin < numBins_; ++bin)
        {
            float re = scFftData_[static_cast<size_t>(bin * 2)];
            float im = scFftData_[static_cast<size_t>(bin * 2 + 1)];
            scMagnitudes_[static_cast<size_t>(bin)] = std::sqrt(re * re + im * im);
        }
    }

    if (pitchDetectEnabled_)
    {
        float detectedHz = pitchDetector_.detect(pitchDetectBuffer_.data(), fftSize_);
        float confidence = pitchDetector_.getConfidence();

        float targetRatio = 1.0f;
        float detectedSemi = 0.0f;
        float targetSemi = 0.0f;
        float correctedSemi = 0.0f;

        if (detectedHz > 0.0f && confidence >= 0.5f)
        {
            detectedSemi = 69.0f + 12.0f * std::log2(detectedHz / 440.0f);
            targetSemi = scale_.findNearestScalePitch(detectedHz);

            if (targetSemi > 0.0f)
            {
                float correction = (targetSemi - detectedSemi) * correctionAmount_;
                correctedSemi = detectedSemi + correction;
                targetRatio = std::pow(2.0f, correction / 12.0f);
            }
        }

        float hopDuration = static_cast<float>(hopSize_) / static_cast<float>(sampleRate_);
        float coeff;
        if (retuneSpeedMs_ <= 0.0f) coeff = 0.0f;
        else coeff = std::exp(-hopDuration / (retuneSpeedMs_ / 1000.0f));

        smoothedRatio_ = coeff * smoothedRatio_ + (1.0f - coeff) * targetRatio;

        pitchDetectedHz_.store(detectedHz, std::memory_order_relaxed);
        pitchDetectedSemi_.store(detectedSemi, std::memory_order_relaxed);
        pitchTargetSemi_.store(targetSemi, std::memory_order_relaxed);
        pitchCorrectedSemi_.store(correctedSemi, std::memory_order_relaxed);
        pitchConfidence_.store(confidence, std::memory_order_relaxed);
    }

    std::copy(magnitudes_.begin(), magnitudes_.begin() + numBins_, eraseMags_.begin());
    std::copy(phases_.begin(), phases_.begin() + numBins_, erasePhases_.begin());

    if (eraser_ != nullptr)
    {
        if (useSidechain)
            eraser_->setExternalDetection(scMagnitudes_.data(), numBins_);
        else
            eraser_->setExternalDetection(nullptr, 0);

        eraser_->process(eraseMags_.data(), numBins_);

        const float* gains = eraser_->getGains();
        std::copy(gains, gains + numBins_, eraseGainsCache_.begin());
    }

    float freqPerBin = static_cast<float>(sampleRate_) / static_cast<float>(fftSize_);
    float expectedPhaseDiff = twoPi * static_cast<float>(hopSize_) / static_cast<float>(fftSize_);

    for (int bin = 0; bin < numBins_; ++bin)
    {
        float phaseDiff = phases_[static_cast<size_t>(bin)] - prevPhases_[static_cast<size_t>(bin)];
        prevPhases_[static_cast<size_t>(bin)] = phases_[static_cast<size_t>(bin)];
        phaseDiff -= static_cast<float>(bin) * expectedPhaseDiff;
        phaseDiff = phaseDiff - twoPi * std::round(phaseDiff / twoPi);
        float trueFreq = static_cast<float>(bin) * freqPerBin
                          + phaseDiff * freqPerBin / expectedPhaseDiff;
        analysisFreqs_[static_cast<size_t>(bin)] = trueFreq;
    }

    float R = smoothedRatio_;
    std::fill(tuneMags_.begin(), tuneMags_.begin() + numBins_, 0.0f);
    std::fill(shiftedFreqs_.begin(), shiftedFreqs_.begin() + numBins_, 0.0f);

    for (int bin = 0; bin < numBins_; ++bin)
    {
        int newBin = static_cast<int>(std::round(static_cast<float>(bin) * R));
        if (newBin >= 0 && newBin < numBins_)
        {
            tuneMags_[static_cast<size_t>(newBin)] += magnitudes_[static_cast<size_t>(bin)];
            shiftedFreqs_[static_cast<size_t>(newBin)] = analysisFreqs_[static_cast<size_t>(bin)] * R;
        }
    }

    for (int bin = 0; bin < numBins_; ++bin)
    {
        float phaseAdvance = shiftedFreqs_[static_cast<size_t>(bin)] / freqPerBin * expectedPhaseDiff;
        synthPhases_[static_cast<size_t>(bin)] += phaseAdvance;
        tunePhases_[static_cast<size_t>(bin)] = synthPhases_[static_cast<size_t>(bin)];
    }

    for (int i = 0; i < numBins_; ++i)
    {
        float eraseRe = eraseMags_[static_cast<size_t>(i)] * std::cos(erasePhases_[static_cast<size_t>(i)]);
        float eraseIm = eraseMags_[static_cast<size_t>(i)] * std::sin(erasePhases_[static_cast<size_t>(i)]);
        float tuneRe  = tuneMags_[static_cast<size_t>(i)]  * std::cos(tunePhases_[static_cast<size_t>(i)]);
        float tuneIm  = tuneMags_[static_cast<size_t>(i)]  * std::sin(tunePhases_[static_cast<size_t>(i)]);

        float cf = spectralCrossfade_;
        float re = eraseRe * (1.0f - cf) + tuneRe * cf;
        float im = eraseIm * (1.0f - cf) + tuneIm * cf;

        blendedMags_[static_cast<size_t>(i)] = std::sqrt(re * re + im * im);
        blendedPhases_[static_cast<size_t>(i)] = std::atan2(im, re);
    }

    for (int bin = 0; bin < numBins_; ++bin)
    {
        float mag = blendedMags_[static_cast<size_t>(bin)];
        float ph  = blendedPhases_[static_cast<size_t>(bin)];
        fftData_[static_cast<size_t>(bin * 2)]     = mag * std::cos(ph);
        fftData_[static_cast<size_t>(bin * 2 + 1)] = mag * std::sin(ph);
    }

    fft_->performRealOnlyInverseTransform(fftData_.data());

    int outStart = outputReadPos_;
    for (int i = 0; i < fftSize_; ++i)
    {
        int idx = (outStart + i) % static_cast<int>(outputBuffer_.size());
        outputBuffer_[static_cast<size_t>(idx)] += fftData_[static_cast<size_t>(i)]
                                                    * window_[static_cast<size_t>(i)]
                                                    * kOLAGain;
    }
}

} // namespace tonalizer
