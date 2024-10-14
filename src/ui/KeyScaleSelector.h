#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace tonalizer {

class TonalizerProcessor;

class KeyScaleSelector : public juce::Component
{
public:
    explicit KeyScaleSelector(TonalizerProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    TonalizerProcessor& processor_;
    juce::AudioProcessorValueTreeState& apvts_;

    juce::Label     keyLabel_;
    juce::Label     scaleLabel_;
    juce::ComboBox  keyCombo_;
    juce::ComboBox  scaleCombo_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyScaleSelector)
};

} // namespace tonalizer
