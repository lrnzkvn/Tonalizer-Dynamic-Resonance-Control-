#include "ui/DotMatrixDisplay.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "model/Parameters.h"
#include "ui/LookAndFeel.h"

namespace tonalizer {

namespace {

constexpr float kMinFrequency = 30.0f;
constexpr float kMaxFrequency = 20000.0f;
constexpr float kMinDb = -60.0f;
constexpr float kMaxDb = 10.0f;

std::array<float, 10> majorFrequencies()
{
    return { 30.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
}

juce::String formatFrequencyLabel(float frequency)
{
    if (frequency >= 1000.0f)
    {
        const float kilo = frequency / 1000.0f;
        return kilo >= 10.0f ? juce::String(static_cast<int>(std::round(kilo))) + "k"
                             : juce::String(kilo, 0) + "k";
    }

    return juce::String(static_cast<int>(std::round(frequency)));
}

} // namespace

DotMatrixDisplay::DotMatrixDisplay(TonalizerProcessor& processor)
    : processor_(processor)
{
    rawSpectrum_.fill(kMinDb);
    rawSpectrumCh1_.fill(kMinDb);
    processedSpectrum_.fill(kMinDb);
    startTimerHz(30);
}

void DotMatrixDisplay::timerCallback()
{
    auto snapshot = processor_.getVisualizationSnapshot();
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(processor_.apvts.getParameter(ParamIDs::crossfade)))
        crossfadeAmount_ = param->get() / 100.0f;

    if (!snapshot.ready)
    {
        for (auto& value : rawSpectrum_)
            value += (kMinDb - value) * 0.12f;

        for (auto& value : rawSpectrumCh1_)
            value += (kMinDb - value) * 0.12f;

        for (auto& value : processedSpectrum_)
            value += (kMinDb - value) * 0.12f;

        meterLevel_ *= 0.9f;
        repaint();
        return;
    }

    if (snapshot.fftSize > 0)
    {
        currentFFTSize_ = snapshot.fftSize;
        currentNumBins_ = std::max(1, snapshot.numBins);
    }

    hasCh1_ = snapshot.numChannels > 1;
    msMode_ = snapshot.msMode;

    float peakDb = kMinDb;
    const int nb = std::min(currentNumBins_, kNumBins);

    for (int i = 0; i < nb; ++i)
    {
        const auto mag = juce::jmax(snapshot.magnitudes[static_cast<size_t>(i)], 1.0e-5f);
        const auto procMag = juce::jmax(snapshot.magnitudes[static_cast<size_t>(i)]
                                            * snapshot.gains[static_cast<size_t>(i)],
                                        1.0e-5f);

        const auto rawDb = juce::jlimit(kMinDb, kMaxDb,
                                        juce::Decibels::gainToDecibels(mag, kMinDb) + 18.0f);
        const auto processedDb = juce::jlimit(kMinDb, kMaxDb,
                                              juce::Decibels::gainToDecibels(procMag, kMinDb) + 18.0f);

        rawSpectrum_[static_cast<size_t>(i)] += (rawDb - rawSpectrum_[static_cast<size_t>(i)]) * 0.30f;
        processedSpectrum_[static_cast<size_t>(i)] += (processedDb - processedSpectrum_[static_cast<size_t>(i)]) * 0.18f;
        peakDb = juce::jmax(peakDb, processedSpectrum_[static_cast<size_t>(i)]);

        if (hasCh1_)
        {
            const auto mag1 = juce::jmax(snapshot.magnitudesCh1[static_cast<size_t>(i)], 1.0e-5f);
            const auto raw1Db = juce::jlimit(kMinDb, kMaxDb,
                                             juce::Decibels::gainToDecibels(mag1, kMinDb) + 18.0f);
            rawSpectrumCh1_[static_cast<size_t>(i)] += (raw1Db - rawSpectrumCh1_[static_cast<size_t>(i)]) * 0.30f;
        }
        else
        {
            rawSpectrumCh1_[static_cast<size_t>(i)] += (kMinDb - rawSpectrumCh1_[static_cast<size_t>(i)]) * 0.20f;
        }
    }
    // Decay any stale entries past the current active band count.
    for (int i = nb; i < kNumBins; ++i)
    {
        rawSpectrum_[static_cast<size_t>(i)] += (kMinDb - rawSpectrum_[static_cast<size_t>(i)]) * 0.20f;
        rawSpectrumCh1_[static_cast<size_t>(i)] += (kMinDb - rawSpectrumCh1_[static_cast<size_t>(i)]) * 0.20f;
        processedSpectrum_[static_cast<size_t>(i)] += (kMinDb - processedSpectrum_[static_cast<size_t>(i)]) * 0.20f;
    }

    const float targetMeter = juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, kMinDb, 3.0f, 0.0f, 1.0f));
    meterLevel_ += (targetMeter - meterLevel_) * 0.18f;
    repaint();
}

float DotMatrixDisplay::frequencyToX(float frequency, juce::Rectangle<float> plotArea) const
{
    const float logMin = std::log10(kMinFrequency);
    const float logMax = std::log10(kMaxFrequency);
    const float t = (std::log10(frequency) - logMin) / (logMax - logMin);
    return plotArea.getX() + plotArea.getWidth() * juce::jlimit(0.0f, 1.0f, t);
}

float DotMatrixDisplay::decibelsToY(float db, juce::Rectangle<float> plotArea) const
{
    return juce::jmap(db, kMaxDb, kMinDb, plotArea.getY(), plotArea.getBottom());
}

float DotMatrixDisplay::sampleSpectrum(const std::array<float, kNumBins>& values, float bin, int radius) const
{
    const int centre = juce::jlimit(0, kNumBins - 1, static_cast<int>(std::round(bin)));
    const int start = juce::jmax(0, centre - radius);
    const int end = juce::jmin(kNumBins - 1, centre + radius);

    float total = 0.0f;
    float weightTotal = 0.0f;

    for (int i = start; i <= end; ++i)
    {
        const float distance = std::abs(static_cast<float>(i) - bin);
        const float weight = 1.0f / (1.0f + distance);
        total += values[static_cast<size_t>(i)] * weight;
        weightTotal += weight;
    }

    return weightTotal > 0.0f ? total / weightTotal : values[static_cast<size_t>(centre)];
}

void DotMatrixDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto meterArea = bounds.removeFromRight(24.0f).reduced(0.0f, 8.0f);
    auto plotArea = bounds.reduced(10.0f, 10.0f);
    plotArea.removeFromRight(14.0f);

    juce::ColourGradient background(juce::Colour(0xff20262d), plotArea.getX(), plotArea.getY(),
                                    juce::Colour(0xff0e1217), plotArea.getRight(), plotArea.getBottom(), false);
    g.setGradientFill(background);
    g.fillRect(plotArea);

    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.fillRect(meterArea);

    const auto majorFreqs = majorFrequencies();
    const auto drawGridLine = [&](float frequency, float alpha, bool major)
    {
        const float x = frequencyToX(frequency, plotArea);
        g.setColour(juce::Colour(0xffd7d8db).withAlpha(alpha));
        g.drawVerticalLine(static_cast<int>(std::round(x)), plotArea.getY(), plotArea.getBottom());

        if (major)
        {
            const auto label = frequency == 20000.0f ? juce::String("20k")
                                                     : formatFrequencyLabel(frequency);
            g.setColour(Colours::panelText.withAlpha(0.78f));
            g.setFont(VoidLookAndFeel::getSansFont(16.0f, false));
            g.drawFittedText(label, juce::Rectangle<int>(static_cast<int>(x) - 18,
                                                         static_cast<int>(plotArea.getY()) - 28, 36, 20),
                             juce::Justification::centred, 1);
        }
    };

    for (int decade = 1; decade <= 4; ++decade)
    {
        const float base = std::pow(10.0f, static_cast<float>(decade));
        for (int mult = 1; mult <= 9; ++mult)
        {
            const float frequency = base * static_cast<float>(mult);
            if (frequency < kMinFrequency || frequency > kMaxFrequency)
                continue;

            const bool isMajor = std::find(majorFreqs.begin(), majorFreqs.end(), frequency) != majorFreqs.end();
            drawGridLine(frequency, isMajor ? 0.25f : 0.10f, isMajor);
        }
    }

    for (float db : { 10.0f, 0.0f, -10.0f, -20.0f, -30.0f, -40.0f, -50.0f })
    {
        const float y = decibelsToY(db, plotArea);
        g.setColour(juce::Colour(0xffd7d8db).withAlpha(db == -20.0f ? 0.16f : 0.10f));
        g.drawHorizontalLine(static_cast<int>(std::round(y)), plotArea.getX(), plotArea.getRight());

        const auto label = db == 10.0f ? juce::String("10") : juce::String(static_cast<int>(db));
        g.setColour(Colours::panelText.withAlpha(0.76f));
        g.setFont(VoidLookAndFeel::getSansFont(15.0f, false));
        g.drawText(label, juce::Rectangle<int>(0, static_cast<int>(y) - 10, 52, 18), juce::Justification::centredRight);
    }

    g.setColour(Colours::panelText.withAlpha(0.90f));
    g.setFont(VoidLookAndFeel::getSansFont(16.0f, false));
    g.drawText("dB", juce::Rectangle<int>(4, static_cast<int>(plotArea.getY()) - 28, 40, 18), juce::Justification::centredLeft);
    g.drawText("kHz", juce::Rectangle<int>(static_cast<int>(plotArea.getRight()) - 36,
                                           static_cast<int>(plotArea.getY()) - 28, 36, 18),
               juce::Justification::centredRight);
    g.drawText("-dB", juce::Rectangle<int>(4, static_cast<int>(plotArea.getBottom()) - 6, 40, 18), juce::Justification::centredLeft);

    juce::Path fillPath;
    juce::Path rawOutline;
    juce::Path ch1Outline;
    juce::Path tunedOutline;

    const int pointCount = 260;
    for (int i = 0; i < pointCount; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(pointCount - 1);
        const float frequency = kMinFrequency * std::pow(kMaxFrequency / kMinFrequency, t);
        const float binWidth = 44100.0f / static_cast<float>(currentFFTSize_);
        const float bin = juce::jlimit(0.0f, static_cast<float>(currentNumBins_ - 1), frequency / binWidth);

        const float rawDb = sampleSpectrum(rawSpectrum_, bin, 3);
        const float processedDb = sampleSpectrum(processedSpectrum_, bin, 18);
        const float featuredDb = juce::jmap(crossfadeAmount_, rawDb, processedDb);
        const float x = frequencyToX(frequency, plotArea);
        const float rawY = decibelsToY(rawDb, plotArea);
        const float tunedY = decibelsToY(featuredDb, plotArea);

        if (i == 0)
        {
            fillPath.startNewSubPath(x, plotArea.getBottom());
            fillPath.lineTo(x, rawY);
            rawOutline.startNewSubPath(x, rawY);
            tunedOutline.startNewSubPath(x, tunedY);
        }
        else
        {
            fillPath.lineTo(x, rawY);
            rawOutline.lineTo(x, rawY);
            tunedOutline.lineTo(x, tunedY);
        }

        if (hasCh1_)
        {
            const float ch1Db = sampleSpectrum(rawSpectrumCh1_, bin, 3);
            const float ch1Y = decibelsToY(ch1Db, plotArea);
            if (i == 0)
                ch1Outline.startNewSubPath(x, ch1Y);
            else
                ch1Outline.lineTo(x, ch1Y);
        }
    }

    fillPath.lineTo(plotArea.getRight(), plotArea.getBottom());
    fillPath.closeSubPath();

    juce::ColourGradient fill(Colours::gold.withAlpha(0.52f), plotArea.getCentreX(), plotArea.getY(),
                              juce::Colour(0x18271a08), plotArea.getCentreX(), plotArea.getBottom(), false);
    g.setGradientFill(fill);
    g.fillPath(fillPath);

    if (hasCh1_)
    {
        // Side in M/S gets a cool teal so it reads as a distinct stream;
        // R in L/R gets a softer warm tint so the pair looks related.
        const auto ch1Col = msMode_ ? juce::Colour(0xff4ac0d6)
                                    : Colours::goldBright.withRotatedHue(0.04f);
        g.setColour(ch1Col.withAlpha(0.55f));
        g.strokePath(ch1Outline, juce::PathStrokeType(1.4f));
    }

    g.setColour(Colours::goldBright.withAlpha(0.52f));
    g.strokePath(rawOutline, juce::PathStrokeType(1.6f));

    g.setColour(Colours::goldGlow);
    g.strokePath(tunedOutline, juce::PathStrokeType(8.0f));
    g.setColour(Colours::goldBright.withAlpha(0.94f));
    g.strokePath(tunedOutline, juce::PathStrokeType(2.6f));

    // Stereo-mode legend in the plot's top-left corner.
    {
        const juce::String primaryLabel = hasCh1_ ? (msMode_ ? "M" : "L") : juce::String();
        const juce::String ch1Label     = hasCh1_ ? (msMode_ ? "S" : "R") : juce::String();
        if (hasCh1_)
        {
            const int legendY = static_cast<int>(plotArea.getY()) + 4;
            const int legendX = static_cast<int>(plotArea.getX()) + 8;
            g.setFont(VoidLookAndFeel::getSansFont(13.0f, false));

            g.setColour(Colours::goldBright.withAlpha(0.90f));
            g.drawText(primaryLabel, juce::Rectangle<int>(legendX, legendY, 14, 14),
                       juce::Justification::centredLeft);

            const auto ch1Col = msMode_ ? juce::Colour(0xff4ac0d6)
                                        : Colours::goldBright.withRotatedHue(0.04f);
            g.setColour(ch1Col.withAlpha(0.90f));
            g.drawText(ch1Label, juce::Rectangle<int>(legendX + 16, legendY, 14, 14),
                       juce::Justification::centredLeft);
        }
    }

    auto meterWell = meterArea.reduced(0.0f, 4.0f);
    g.setColour(juce::Colour(0xff0b0d11));
    g.fillRect(meterWell);

    const int segmentCount = 52;
    const float segmentGap = 1.6f;
    const float segmentHeight = (meterWell.getHeight() - segmentGap * (segmentCount - 1)) / static_cast<float>(segmentCount);
    const int litSegments = static_cast<int>(std::round(meterLevel_ * static_cast<float>(segmentCount)));

    for (int i = 0; i < segmentCount; ++i)
    {
        const float y = meterWell.getBottom() - (i + 1) * segmentHeight - i * segmentGap;
        auto segment = juce::Rectangle<float>(meterWell.getX() + 4.0f, y, meterWell.getWidth() - 8.0f, segmentHeight);
        const bool lit = i < litSegments;
        const float tint = static_cast<float>(i) / static_cast<float>(segmentCount - 1);
        auto segmentColour = juce::Colour(0xff45484f);
        if (lit)
            segmentColour = Colours::gold.interpolatedWith(Colours::goldBright, tint * 0.45f + 0.15f);

        g.setColour(segmentColour.withAlpha(lit ? 0.96f : 0.22f));
        g.fillRoundedRectangle(segment, 1.0f);
    }

    for (int i = 0; i < 3; ++i)
    {
        const auto ledColour = i == 0 ? juce::Colour(0xff2d8b45)
                                      : (i == 1 ? juce::Colour(0xff6f5d21) : juce::Colour(0xff7f2a23));
        g.setColour(ledColour.withAlpha(0.85f));
        g.fillEllipse(meterWell.getCentreX() - 11.0f + static_cast<float>(i) * 10.0f,
                      plotArea.getY() - 20.0f, 4.0f, 4.0f);
    }

    g.setColour(Colours::panelText.withAlpha(0.76f));
    g.setFont(VoidLookAndFeel::getSansFont(13.0f, false));
    g.drawText("L", juce::Rectangle<int>(static_cast<int>(meterArea.getX()) - 1,
                                         static_cast<int>(plotArea.getBottom()) + 2, 14, 14),
               juce::Justification::centred);
    g.drawText("O", juce::Rectangle<int>(static_cast<int>(meterArea.getRight()) - 14,
                                         static_cast<int>(plotArea.getBottom()) + 2, 14, 14),
               juce::Justification::centred);
}

} // namespace tonalizer
