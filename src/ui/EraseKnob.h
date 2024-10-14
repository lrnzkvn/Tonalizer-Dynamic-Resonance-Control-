#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace tonalizer {

class EraseKnob : public juce::Component
{
public:
    EraseKnob(const juce::String& labelText,
              juce::AudioProcessorValueTreeState& apvts,
              const juce::String& paramID);

    void resized() override;

    juce::Slider& getSlider() { return slider_; }
    void setArcColour(juce::Colour colour);
    void setFrameVisible(bool shouldBeVisible);

private:
    juce::Slider slider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EraseKnob)
};

} // namespace tonalizer
