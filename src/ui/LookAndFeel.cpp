#include "ui/LookAndFeel.h"

#include <cmath>

namespace tonalizer {

namespace {

const juce::Identifier kShowKnobFrameProperty { "tonalizerShowKnobFrame" };
const juce::Identifier kShowControlVisualsProperty { "tonalizerShowControlVisuals" };

float getKnobAngle(float sliderPos, float startAngle, float endAngle)
{
    return startAngle + sliderPos * (endAngle - startAngle);
}

bool shouldDrawKnobFrame(juce::Slider& slider)
{
    return static_cast<bool>(slider.getProperties().getWithDefault(kShowKnobFrameProperty, true));
}

bool shouldDrawControlVisuals(juce::Component& component)
{
    return static_cast<bool>(component.getProperties().getWithDefault(kShowControlVisualsProperty, true));
}

} // namespace

void TonalizerLookAndFeel::setKnobFrameVisible(juce::Slider& slider, bool shouldBeVisible)
{
    slider.getProperties().set(kShowKnobFrameProperty, shouldBeVisible);
}

void TonalizerLookAndFeel::setControlVisualsVisible(juce::Component& component, bool shouldBeVisible)
{
    component.getProperties().set(kShowControlVisualsProperty, shouldBeVisible);
}

TonalizerLookAndFeel::TonalizerLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, Colours::widgetBlack);
    setColour(juce::ComboBox::textColourId, Colours::panelText);
    setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff111317));
    setColour(juce::PopupMenu::textColourId, Colours::panelText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Colours::gold.withAlpha(0.18f));
    setColour(juce::PopupMenu::highlightedTextColourId, Colours::goldBright);
}

void TonalizerLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider& slider)
{
    if (! shouldDrawControlVisuals(slider))
        return;

    const bool drawFrame = shouldDrawKnobFrame(slider);
    auto area = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                       static_cast<float>(w), static_cast<float>(h)).reduced(4.0f);
    const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float angle = getKnobAngle(sliderPos, startAngle, endAngle);
    const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId).interpolatedWith(Colours::gold, 0.4f);

    if (!drawFrame)
    {
        const float pointerInset = radius * 0.08f;
        const float pointerLength = radius * 0.76f;
        const float sx = cx + std::cos(angle - juce::MathConstants<float>::halfPi) * pointerInset;
        const float sy = cy + std::sin(angle - juce::MathConstants<float>::halfPi) * pointerInset;
        const float px = cx + std::cos(angle - juce::MathConstants<float>::halfPi) * pointerLength;
        const float py = cy + std::sin(angle - juce::MathConstants<float>::halfPi) * pointerLength;
        const float pointerThickness = juce::jmax(2.1f, radius * 0.075f);

        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.drawLine(sx + 1.0f, sy + 1.0f, px + 1.0f, py + 1.0f, pointerThickness + 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.92f));
        g.drawLine(sx, sy, px, py, pointerThickness);
        g.setColour(juce::Colours::white.withAlpha(0.28f));
        g.drawLine(sx - 0.5f, sy - 0.5f, px - 0.5f, py - 0.5f, 1.0f);
        return;
    }

    if (drawFrame)
    {
        juce::ColourGradient shadow(juce::Colours::black.withAlpha(0.55f), cx, cy + radius * 0.15f,
                                    juce::Colours::transparentBlack, cx, cy + radius * 1.35f, true);
        g.setGradientFill(shadow);
        g.fillEllipse(cx - radius * 0.98f, cy - radius * 0.82f, radius * 1.96f, radius * 1.96f);

        const float tickOuter = radius * 1.06f;
        const float tickInner = radius * 0.88f;
        for (int i = 0; i <= 10; ++i)
        {
            const float t = static_cast<float>(i) / 10.0f;
            const float tickAngle = getKnobAngle(t, startAngle, endAngle);
            const bool active = t <= sliderPos + 0.0001f;
            const float x1 = cx + std::cos(tickAngle - juce::MathConstants<float>::halfPi) * tickInner;
            const float y1 = cy + std::sin(tickAngle - juce::MathConstants<float>::halfPi) * tickInner;
            const float x2 = cx + std::cos(tickAngle - juce::MathConstants<float>::halfPi) * tickOuter;
            const float y2 = cy + std::sin(tickAngle - juce::MathConstants<float>::halfPi) * tickOuter;

            g.setColour((active ? accent : Colours::widgetEdge).withAlpha(active ? 0.8f : 0.45f));
            g.drawLine(x1, y1, x2, y2, i % 5 == 0 ? 1.6f : 1.1f);
        }

        juce::Path ring;
        ring.addEllipse(cx - radius * 0.90f, cy - radius * 0.90f, radius * 1.80f, radius * 1.80f);
        g.setColour(Colours::goldShadow.withAlpha(0.95f));
        g.strokePath(ring, juce::PathStrokeType(radius * 0.18f));

        juce::ColourGradient ringLight(juce::Colour(0xffe3d097), cx - radius * 0.25f, cy - radius * 0.8f,
                                       juce::Colour(0xff6a5631), cx + radius * 0.35f, cy + radius * 0.85f, false);
        ringLight.addColour(0.45, juce::Colour(0xffaa8b55));
        g.setGradientFill(ringLight);
        g.strokePath(ring, juce::PathStrokeType(radius * 0.11f));
    }

    const float bodyScale = 0.70f;
    const float capScale = 0.32f;

    {
        juce::ColourGradient body(Colours::widgetTop, cx - radius * 0.35f, cy - radius * 0.8f,
                                  Colours::widgetBlack, cx + radius * 0.25f, cy + radius * 0.9f, false);
        body.addColour(0.48, juce::Colour(0xff272a30));
        g.setGradientFill(body);
        g.fillEllipse(cx - radius * bodyScale, cy - radius * bodyScale,
                      radius * bodyScale * 2.0f, radius * bodyScale * 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawEllipse(cx - radius * bodyScale, cy - radius * bodyScale,
                      radius * bodyScale * 2.0f, radius * bodyScale * 2.0f, 1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.drawEllipse(cx - radius * bodyScale * 0.89f, cy - radius * bodyScale * 0.89f,
                      radius * bodyScale * 1.78f, radius * bodyScale * 1.78f, 1.0f);
    }

    {
        juce::ColourGradient cap(juce::Colour(0xff4b4f55), cx - radius * 0.16f, cy - radius * 0.26f,
                                 juce::Colour(0xff111216), cx + radius * 0.18f, cy + radius * 0.3f, false);
        g.setGradientFill(cap);
        g.fillEllipse(cx - radius * capScale, cy - radius * capScale,
                      radius * capScale * 2.0f, radius * capScale * 2.0f);
    }

    {
        const float pointerLength = radius * 0.62f;
        const float px = cx + std::cos(angle - juce::MathConstants<float>::halfPi) * pointerLength;
        const float py = cy + std::sin(angle - juce::MathConstants<float>::halfPi) * pointerLength;

        g.setColour(juce::Colours::white.withAlpha(0.78f));
        g.drawLine(cx, cy, px, py, juce::jmax(2.0f, radius * 0.06f));
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawLine(cx, cy - 1.0f, px, py - 1.0f, 1.0f);
    }
}

void TonalizerLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                                        float sliderPos, float, float,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (! shouldDrawControlVisuals(slider))
        return;

    (void) slider;

    if (style != juce::Slider::LinearHorizontal)
    {
        VoidLookAndFeel::drawLinearSlider(g, x, y, w, h, sliderPos, 0.0f, 0.0f, style, slider);
        return;
    }

    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                         static_cast<float>(w), static_cast<float>(h)).reduced(6.0f, 8.0f);
    const float shellHeight = juce::jlimit(32.0f, 42.0f, bounds.getHeight() * 0.54f);
    auto shell = juce::Rectangle<float>(bounds.getX(),
                                        bounds.getCentreY() - shellHeight * 0.5f,
                                        bounds.getWidth(), shellHeight);

    const float innerMarginX = juce::jlimit(18.0f, 26.0f, shell.getWidth() * 0.055f);
    const float innerTrackHeight = juce::jlimit(6.0f, 10.0f, shell.getHeight() * 0.18f);
    const auto innerTrack = juce::Rectangle<float>(shell.getX() + innerMarginX,
                                                   shell.getCentreY() - innerTrackHeight * 0.5f,
                                                   shell.getWidth() - innerMarginX * 2.0f,
                                                   innerTrackHeight);

    const float thumbWidth = juce::jlimit(28.0f, 34.0f, shell.getHeight() * 0.78f);
    const float thumbHeight = shell.getHeight() * 0.92f;
    const float thumbX = juce::jlimit(shell.getX() + thumbWidth * 0.5f + 8.0f,
                                      shell.getRight() - thumbWidth * 0.5f - 8.0f,
                                      sliderPos);

    const float glowGap = thumbWidth * 0.34f;
    {
        juce::Graphics::ScopedSaveState clipState(g);
        g.reduceClipRegion(shell.reduced(1.0f).toNearestInt());

        g.setColour(juce::Colour(0xff06080b).withAlpha(0.94f));
        g.fillRoundedRectangle(innerTrack, innerTrack.getHeight() * 0.5f);
        g.setColour(juce::Colours::black.withAlpha(0.50f));
        g.drawRoundedRectangle(innerTrack, innerTrack.getHeight() * 0.5f, 0.9f);

        if (thumbX > innerTrack.getX())
        {
            auto leftGlow = innerTrack.withRight(thumbX - glowGap);
            if (leftGlow.getWidth() > 2.0f)
            {
                juce::ColourGradient gold(Colours::goldBright.withAlpha(0.94f), leftGlow.getX(), leftGlow.getCentreY(),
                                          Colours::gold.withAlpha(0.14f), leftGlow.getRight(), leftGlow.getCentreY(), false);
                g.setGradientFill(gold);
                g.fillRoundedRectangle(leftGlow, leftGlow.getHeight() * 0.5f);

                juce::ColourGradient halo(Colours::goldGlow.withAlpha(0.72f), leftGlow.getCentreX(), leftGlow.getCentreY(),
                                          juce::Colours::transparentBlack, leftGlow.getCentreX(), leftGlow.getBottom() + 6.0f, true);
                g.setGradientFill(halo);
                g.fillRoundedRectangle(leftGlow.expanded(2.0f, 4.0f), leftGlow.getHeight() * 0.75f);
            }
        }

        if (thumbX < innerTrack.getRight())
        {
            auto rightGlow = innerTrack.withX(thumbX + glowGap);
            if (rightGlow.getWidth() > 2.0f)
            {
                juce::ColourGradient blue(Colours::blue.withAlpha(0.14f), rightGlow.getX(), rightGlow.getCentreY(),
                                          Colours::blueBright.withAlpha(0.86f), rightGlow.getRight(), rightGlow.getCentreY(), false);
                g.setGradientFill(blue);
                g.fillRoundedRectangle(rightGlow, rightGlow.getHeight() * 0.5f);

                juce::ColourGradient halo(Colours::blueGlow.withAlpha(0.64f), rightGlow.getCentreX(), rightGlow.getCentreY(),
                                          juce::Colours::transparentBlack, rightGlow.getCentreX(), rightGlow.getBottom() + 6.0f, true);
                g.setGradientFill(halo);
                g.fillRoundedRectangle(rightGlow.expanded(2.0f, 4.0f), rightGlow.getHeight() * 0.75f);
            }
        }
    }

    auto thumb = juce::Rectangle<float>(thumbX - thumbWidth * 0.5f, shell.getCentreY() - thumbHeight * 0.5f,
                                        thumbWidth, thumbHeight);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillRoundedRectangle(thumb.translated(0.0f, 2.0f), 7.5f);

    juce::ColourGradient thumbBody(juce::Colour(0xff2f353b), thumb.getCentreX(), thumb.getY(),
                                   juce::Colour(0xff06080b), thumb.getCentreX(), thumb.getBottom(), false);
    thumbBody.addColour(0.35, juce::Colour(0xff414851));
    g.setGradientFill(thumbBody);
    g.fillRoundedRectangle(thumb, 7.5f);

    auto thumbHighlight = thumb;
    thumbHighlight.removeFromBottom(thumb.getHeight() * 0.60f);
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    g.fillRoundedRectangle(thumbHighlight, 7.5f);

    const auto grooveX = thumb.getCentreX();
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawLine(grooveX + 0.5f, thumb.getY() + 5.0f, grooveX + 0.5f, thumb.getBottom() - 5.0f, 1.2f);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawLine(grooveX - 0.5f, thumb.getY() + 5.0f, grooveX - 0.5f, thumb.getBottom() - 5.0f, 0.9f);
}

void TonalizerLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool isDown,
                                    int, int, int, int, juce::ComboBox& box)
{
    if (! shouldDrawControlVisuals(box))
        return;

    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)).reduced(0.5f);
    const float radius = juce::jmin(7.0f, bounds.getHeight() * 0.28f);

    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), radius);

    juce::ColourGradient fill(juce::Colour(0xff2e2f34), bounds.getCentreX(), bounds.getY(),
                              juce::Colour(0xff131518), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(bounds, radius);

    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(bounds.removeFromTop(bounds.getHeight() * 0.45f), radius);

    const auto outline = (isDown || box.isMouseOver()) ? Colours::gold.withAlpha(0.85f)
                                                        : Colours::widgetEdge.withAlpha(0.85f);
    g.setColour(outline);
    g.drawRoundedRectangle(juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(w) - 1.0f, static_cast<float>(h) - 1.0f),
                           radius, 1.0f);
}

void TonalizerLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    if (! shouldDrawControlVisuals(box))
    {
        label.setBounds(0, 0, 0, 0);
        return;
    }

    VoidLookAndFeel::positionComboBoxText(box, label);
}

void TonalizerLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int w, int h)
{
    juce::ColourGradient fill(juce::Colour(0xff17191d), 0.0f, 0.0f,
                              juce::Colour(0xff0e1013), 0.0f, static_cast<float>(h), false);
    g.setGradientFill(fill);
    g.fillAll();
    g.setColour(Colours::widgetEdge.withAlpha(0.9f));
    g.drawRect(0, 0, w, h, 1);
}

void TonalizerLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isTicked, bool, const juce::String& text,
                                         const juce::String&, const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour(Colours::widgetEdge.withAlpha(0.35f));
        g.drawHorizontalLine(area.getCentreY(), static_cast<float>(area.getX() + 10),
                             static_cast<float>(area.getRight() - 10));
        return;
    }

    if (isHighlighted)
    {
        juce::ColourGradient hi(Colours::gold.withAlpha(0.22f), static_cast<float>(area.getX()), static_cast<float>(area.getY()),
                                Colours::gold.withAlpha(0.06f), static_cast<float>(area.getRight()), static_cast<float>(area.getBottom()), false);
        g.setGradientFill(hi);
        g.fillRect(area);
    }

    auto textColour = isActive ? Colours::panelText : Colours::dimText;
    if (isHighlighted)
        textColour = Colours::goldBright;

    g.setColour(textColour);
    auto font = getPopupMenuFont();
    if (isTicked)
        font = font.boldened();
    g.setFont(font);
    g.drawText(text, area.reduced(12, 0), juce::Justification::centredLeft, true);
}

juce::Font TonalizerLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return VoidLookAndFeel::getSansFont(static_cast<float>(box.getHeight()) * 0.44f, false);
}

juce::Font TonalizerLookAndFeel::getPopupMenuFont()
{
    return VoidLookAndFeel::getSansFont(15.0f, false);
}

} // namespace tonalizer
