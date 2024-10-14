#pragma once

#include <array>

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "ui/DotMatrixDisplay.h"
#include "ui/EraseKnob.h"
#include "ui/KeyScaleSelector.h"
#include "ui/LookAndFeel.h"

namespace tonalizer {

class BandMarkerOverlay;

class TonalizerEditor : public juce::AudioProcessorEditor,
                    private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit TonalizerEditor(TonalizerProcessor& processor);
    ~TonalizerEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    enum class Face { Tune, Resonance };
    void setFace(Face face);
    Face getFace() const { return currentFace_; }

    void openBandPopup(int bandIndex);

private:
    TonalizerProcessor& processor_;
    TonalizerLookAndFeel lookAndFeel_;
    juce::ComponentBoundsConstrainer constrainer_;
    juce::Image referenceGui_;
    juce::Image mainLogo_;

    Face currentFace_ { Face::Tune };

    // Face toggle strip.
    juce::TextButton tuneFaceBtn_;
    juce::TextButton resonanceFaceBtn_;

    // Shared across faces.
    juce::TextButton bypassButton_;
    DotMatrixDisplay dotMatrix_;

    // Tune face.
    juce::Slider crossfadeSlider_;
    EraseKnob eraseKnob_;
    EraseKnob sharpnessKnob_;
    EraseKnob dryWetKnob_;
    EraseKnob retuneKnob_;
    EraseKnob correctionKnob_;
    KeyScaleSelector keyScaleSelector_;

    juce::Label eraseLabel_, sharpnessLabel_, dryWetLabel_, retuneLabel_, correctionLabel_;
    juce::Label crossfadeLabel_;

    // Resonance face — top strip.
    juce::TextButton deltaBtn_;
    juce::TextButton sidechainBtn_;
    juce::TextButton msBtn_;
    juce::ComboBox   qualityCombo_;
    juce::ComboBox   offlineQualityCombo_;

    // Resonance face — knob row 1 (detection shape).
    EraseKnob depthKnob_;
    EraseKnob thresholdKnob_;
    EraseKnob ratioKnob_;
    EraseKnob kneeKnob_;
    EraseKnob selectivityKnob_;
    EraseKnob softHardKnob_;
    juce::Label depthLabel_, thresholdLabel_, ratioLabel_, kneeLabel_, selectivityLabel_, softHardLabel_;

    // Resonance face — knob row 2 (dynamics + stereo).
    EraseKnob attackKnob_;
    EraseKnob releaseKnob_;
    EraseKnob crossfeedKnob_;
    juce::TextButton stereoLinkBtn_;
    juce::Label attackLabel_, releaseLabel_, crossfeedLabel_;

    // Resonance face — 6 band markers overlaid on the spectrogram.
    std::unique_ptr<BandMarkerOverlay> bandOverlay_;

    // Attachments.
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   bypassAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   crossfadeAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   deltaAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   sidechainAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   msAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> qualityAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> offlineQualityAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   stereoLinkAttachment_;

    juce::Rectangle<int> bypassIndicatorBounds_;
    juce::Rectangle<int> resonanceHotspotBounds_;

    void updateFaceVisibility();
    void parameterChanged(const juce::String& id, float newValue) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TonalizerEditor)
};

} // namespace tonalizer
