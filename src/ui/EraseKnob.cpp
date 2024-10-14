#include "ui/EraseKnob.h"

#include "ui/LookAndFeel.h"

namespace tonalizer {

EraseKnob::EraseKnob(const juce::String& labelText,
                     juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramID)
{
    slider_.setName(labelText);
    slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider_.setRotaryParameters(juce::degreesToRadians(225.0f),
                                juce::degreesToRadians(495.0f), true);
    slider_.setMouseDragSensitivity(230);
    slider_.setColour(juce::Slider::rotarySliderFillColourId, Colours::gold);
    slider_.setColour(juce::Slider::rotarySliderOutlineColourId, Colours::widgetBlack);
    addAndMakeVisible(slider_);

    attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramID, slider_);
}

void EraseKnob::resized()
{
    slider_.setBounds(getLocalBounds());
}

void EraseKnob::setArcColour(juce::Colour colour)
{
    slider_.setColour(juce::Slider::rotarySliderFillColourId, colour);
    slider_.repaint();
}

void EraseKnob::setFrameVisible(bool shouldBeVisible)
{
    TonalizerLookAndFeel::setKnobFrameVisible(slider_, shouldBeVisible);
    slider_.repaint();
}

} // namespace tonalizer
