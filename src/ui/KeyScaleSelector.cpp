#include "ui/KeyScaleSelector.h"

#include <cmath>

#include "PluginProcessor.h"
#include "model/MusicalScale.h"
#include "model/Parameters.h"
#include "ui/LookAndFeel.h"

namespace tonalizer {

namespace {

constexpr int kReferenceWidth = 1145;
constexpr int kReferenceHeight = 768;

juce::Rectangle<int> scaleRect(int x, int y, int w, int h, juce::Component& component)
{
    const float sx = static_cast<float>(component.getWidth()) / static_cast<float>(kReferenceWidth);
    const float sy = static_cast<float>(component.getHeight()) / static_cast<float>(kReferenceHeight);

    return {
        static_cast<int>(std::round(static_cast<float>(x) * sx)),
        static_cast<int>(std::round(static_cast<float>(y) * sy)),
        static_cast<int>(std::round(static_cast<float>(w) * sx)),
        static_cast<int>(std::round(static_cast<float>(h) * sy))
    };
}

} // namespace

KeyScaleSelector::KeyScaleSelector(TonalizerProcessor& processor)
    : processor_(processor), apvts_(processor.apvts)
{
    setInterceptsMouseClicks(false, true);

    keyLabel_.setText("Key", juce::dontSendNotification);
    keyLabel_.setFont(VoidLookAndFeel::getSansFont(14.0f, true));
    keyLabel_.setColour(juce::Label::textColourId, Colours::panelText.withAlpha(0.85f));
    keyLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(keyLabel_);

    scaleLabel_.setText("Scale", juce::dontSendNotification);
    scaleLabel_.setFont(VoidLookAndFeel::getSansFont(14.0f, true));
    scaleLabel_.setColour(juce::Label::textColourId, Colours::panelText.withAlpha(0.85f));
    scaleLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(scaleLabel_);

    keyCombo_.addItemList({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    keyCombo_.setJustificationType(juce::Justification::centred);
    TonalizerLookAndFeel::setControlVisualsVisible(keyCombo_, false);
    keyCombo_.onChange = [this] { repaint(); };
    addAndMakeVisible(keyCombo_);

    int itemId = 1;
    auto addSection = [&](const juce::String& title)
    {
        scaleCombo_.addSeparator();
        scaleCombo_.addSectionHeading(title);
    };

    const auto& names = MusicalScale::getScaleNames();
    scaleCombo_.addSectionHeading("Western Modes");
    for (int i = 0; i <= 6; ++i)
        scaleCombo_.addItem(juce::String(names[static_cast<size_t>(i)]), itemId++);
    addSection("Minor Variants");
    for (int i = 7; i <= 12; ++i)
        scaleCombo_.addItem(juce::String(names[static_cast<size_t>(i)]), itemId++);
    addSection("Pentatonic & Blues");
    for (int i = 13; i <= 16; ++i)
        scaleCombo_.addItem(juce::String(names[static_cast<size_t>(i)]), itemId++);
    addSection("Jazz");
    for (int i = 17; i <= 23; ++i)
        scaleCombo_.addItem(juce::String(names[static_cast<size_t>(i)]), itemId++);
    addSection("Middle Eastern");
    for (int i = 24; i <= 27; ++i)
        scaleCombo_.addItem(juce::String(names[static_cast<size_t>(i)]), itemId++);
    addSection("East Asian");
    for (int i = 28; i <= 32; ++i)
        scaleCombo_.addItem(juce::String(names[static_cast<size_t>(i)]), itemId++);
    addSection("Indian");
    for (int i = 33; i <= 35; ++i)
        scaleCombo_.addItem(juce::String(names[static_cast<size_t>(i)]), itemId++);
    addSection("African");
    scaleCombo_.addItem(juce::String(names[36]), itemId++);
    addSection("Experimental");
    for (int i = 37; i <= 40; ++i)
        scaleCombo_.addItem(juce::String(names[static_cast<size_t>(i)]), itemId++);
    addSection("Special");
    scaleCombo_.addItem(juce::String(names[41]), itemId++);
    scaleCombo_.setJustificationType(juce::Justification::centred);
    TonalizerLookAndFeel::setControlVisualsVisible(scaleCombo_, false);
    scaleCombo_.onChange = [this] { repaint(); };
    addAndMakeVisible(scaleCombo_);

    keyAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, ParamIDs::rootNote, keyCombo_);
    scaleAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, ParamIDs::scaleType, scaleCombo_);

    keyLabel_.setVisible(false);
    scaleLabel_.setVisible(false);
}

void KeyScaleSelector::paint(juce::Graphics& g)
{
    const auto drawSelectedText = [&](juce::Rectangle<int> area, const juce::String& text)
    {
        g.setFont(VoidLookAndFeel::getSansFont(static_cast<float>(area.getHeight()) * 0.58f, false));
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.drawText(text, area.translated(0, 1), juce::Justification::centred, true);
        g.setColour(Colours::goldBright.withAlpha(0.92f));
        g.drawText(text, area, juce::Justification::centred, true);
    };

    drawSelectedText(keyCombo_.getBounds(), keyCombo_.getText());
    drawSelectedText(scaleCombo_.getBounds(), scaleCombo_.getText());
}

void KeyScaleSelector::resized()
{
    keyCombo_.setBounds(scaleRect(227, 662, 70, 36, *this));
    scaleCombo_.setBounds(scaleRect(857, 662, 176, 36, *this));
}

} // namespace tonalizer
