#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VoidLookAndFeel.h"

namespace tonalizer {

using KillGeometry::VoidLookAndFeel;

namespace Colours {
inline const juce::Colour panelText   { 0xffd7d0c2 };
inline const juce::Colour dimText     { 0xff988f80 };
inline const juce::Colour gold        { 0xffd0b16e };
inline const juce::Colour goldBright  { 0xfff6e29e };
inline const juce::Colour goldShadow  { 0xff6d5528 };
inline const juce::Colour goldGlow    { 0x80dcb96e };
inline const juce::Colour blue        { 0xff6f9fc6 };
inline const juce::Colour blueBright  { 0xff9dd3ff };
inline const juce::Colour blueGlow    { 0x80689fd1 };
inline const juce::Colour cavity      { 0xff11161c };
inline const juce::Colour cavityEdge  { 0xff252c33 };
inline const juce::Colour widgetBlack { 0xff090b0f };
inline const juce::Colour widgetTop   { 0xff34373c };
inline const juce::Colour widgetMid   { 0xff1b1e22 };
inline const juce::Colour widgetEdge  { 0xff6d665c };
} // namespace Colours

class TonalizerLookAndFeel : public VoidLookAndFeel
{
public:
    static void setKnobFrameVisible(juce::Slider&, bool shouldBeVisible);
    static void setControlVisualsVisible(juce::Component&, bool shouldBeVisible);

    TonalizerLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    void drawComboBox(juce::Graphics&, int w, int h, bool isDown,
                      int bx, int by, int bw, int bh, juce::ComboBox&) override;

    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

    void drawPopupMenuBackground(juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutText, const juce::Drawable* icon,
                           const juce::Colour* textColour) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

} // namespace tonalizer
