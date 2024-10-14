#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

#include <BinaryData.h>

#include "model/Parameters.h"

namespace tonalizer {

namespace {

constexpr int kReferenceWidth = 1145;
constexpr int kReferenceHeight = 768;
constexpr int kDefaultEditorWidth = kReferenceWidth;
constexpr int kDefaultEditorHeight = kReferenceHeight;
constexpr float kMinimumScale = 0.5f;
constexpr float kMaximumScale = 2.0f;

constexpr float kMinFrequency = 30.0f;
constexpr float kMaxFrequency = 20000.0f;
constexpr float kMarkerMinDb = -24.0f;
constexpr float kMarkerMaxDb = 24.0f;

juce::Rectangle<int> getDefaultEditorBounds()
{
    return { 0, 0, kDefaultEditorWidth, kDefaultEditorHeight };
}

bool isLegacyEditorSize(juce::Point<int> size)
{
    return size == juce::Point<int>(900, 604)
        || size == juce::Point<int>(1264, 848)
        || size == juce::Point<int>(kReferenceWidth, kReferenceHeight);
}

juce::Image loadReferenceGui()
{
    return juce::ImageCache::getFromMemory(BinaryData::reference_gui_png,
                                           BinaryData::reference_gui_pngSize);
}

juce::Image loadMainLogo()
{
    return juce::ImageCache::getFromMemory(BinaryData::main_logo_png,
                                           BinaryData::main_logo_pngSize);
}

bool shouldDrawReferenceGui(const juce::Image& image)
{
    return image.isValid();
}

void drawReferenceBypassControl(juce::Graphics& g, juce::Rectangle<float> area,
                                bool active, bool hovered)
{
    const auto ledBounds = juce::Rectangle<float>(area.getX() + 3.0f, area.getCentreY() - 6.5f, 13.0f, 13.0f);
    const auto ledColour = active ? juce::Colour(0xff69ff6a) : juce::Colour(0xff263122);
    g.setColour(ledColour.withAlpha(active ? 0.38f : 0.12f));
    g.fillEllipse(ledBounds.expanded(7.0f));
    g.setColour(ledColour.withAlpha(active ? 0.96f : 0.48f));
    g.fillEllipse(ledBounds);

    auto button = juce::Rectangle<float>(area.getRight() - 30.0f, area.getY() + 1.0f, 28.0f, 28.0f);
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(button.expanded(1.0f).translated(0.0f, 1.0f));

    juce::ColourGradient rim(juce::Colour(0xffeddc9a), button.getCentreX(), button.getY(),
                             juce::Colour(0xff6d5629), button.getCentreX(), button.getBottom(), false);
    rim.addColour(0.38, juce::Colour(0xffc8aa62));
    g.setGradientFill(rim);
    g.fillEllipse(button);

    auto inner = button.reduced(3.0f);
    juce::ColourGradient center(active ? juce::Colour(0xfff8e8a8) : juce::Colour(0xffd2b87a),
                                inner.getCentreX() - 2.0f, inner.getY(),
                                juce::Colour(0xff72582c), inner.getCentreX() + 2.0f, inner.getBottom(), false);
    g.setGradientFill(center);
    g.fillEllipse(inner);

    auto highlight = inner;
    highlight.removeFromBottom(inner.getHeight() * 0.58f);
    g.setColour(juce::Colours::white.withAlpha(hovered ? 0.22f : 0.11f));
    g.fillEllipse(highlight);
}

void drawReferenceHotspotHint(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(Colours::goldGlow.withAlpha(0.16f));
    g.drawEllipse(area.reduced(area.getWidth() * 0.18f), 1.5f);
    g.setColour(Colours::goldBright.withAlpha(0.36f));
    g.drawEllipse(area.reduced(area.getWidth() * 0.28f), 1.0f);
}

float normalisedSliderValue(const juce::Slider& slider)
{
    const auto range = slider.getRange();
    const auto span = range.getLength();

    if (span <= 0.0)
        return 0.0f;

    return juce::jlimit(0.0f, 1.0f,
                        static_cast<float>((slider.getValue() - range.getStart()) / span));
}

float rotaryAngleForValue(float normalisedValue)
{
    const auto start = juce::degreesToRadians(225.0f);
    const auto end = juce::degreesToRadians(495.0f);
    return start + normalisedValue * (end - start) - juce::MathConstants<float>::halfPi;
}

void drawReferenceKnobIndicator(juce::Graphics& g, juce::Rectangle<float> bounds,
                                float normalisedValue, float innerScale, float outerScale)
{
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float angle = rotaryAngleForValue(normalisedValue);

    const float inner = radius * innerScale;
    const float outer = radius * outerScale;

    const float x1 = cx + std::cos(angle) * inner;
    const float y1 = cy + std::sin(angle) * inner;
    const float x2 = cx + std::cos(angle) * outer;
    const float y2 = cy + std::sin(angle) * outer;

    const float thickness = juce::jmax(2.0f, radius * 0.08f);
    g.setColour(Colours::goldGlow.withAlpha(0.45f));
    g.drawLine(x1, y1, x2, y2, thickness + 3.0f);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.drawLine(x1 + 1.2f, y1 + 1.2f, x2 + 1.2f, y2 + 1.2f, thickness + 0.6f);
    g.setColour(juce::Colour(0xfff2ede0).withAlpha(0.96f));
    g.drawLine(x1, y1, x2, y2, thickness);
    g.setColour(juce::Colours::white.withAlpha(0.28f));
    g.drawLine(x1 - 0.6f, y1 - 0.6f, x2 - 0.6f, y2 - 0.6f, 1.0f);

    const float dotRadius = juce::jmax(2.3f, radius * 0.08f);
    g.setColour(juce::Colours::white.withAlpha(0.20f));
    g.fillEllipse(x2 - dotRadius - 1.0f, y2 - dotRadius - 1.0f,
                  (dotRadius + 1.0f) * 2.0f, (dotRadius + 1.0f) * 2.0f);
    g.setColour(juce::Colour(0xfff6f0e4).withAlpha(0.92f));
    g.fillEllipse(x2 - dotRadius, y2 - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}

void drawReferenceCrossfadeIndicator(juce::Graphics& g, juce::Rectangle<float> bounds,
                                     float normalisedValue)
{
    auto track = bounds.reduced(bounds.getWidth() * 0.06f, bounds.getHeight() * 0.24f);
    const float thumbWidth = juce::jlimit(36.0f, 52.0f, bounds.getHeight() * 0.48f);
    const float thumbHeight = juce::jlimit(52.0f, 74.0f, bounds.getHeight() * 0.76f);
    const float x = juce::jmap(normalisedValue,
                               track.getX() + thumbWidth * 0.5f,
                               track.getRight() - thumbWidth * 0.5f);
    const auto thumb = juce::Rectangle<float>(x - thumbWidth * 0.5f,
                                              bounds.getCentreY() - thumbHeight * 0.5f,
                                              thumbWidth, thumbHeight);

    auto leftGlow = juce::Rectangle<float>(track.getX(), track.getCentreY() - 5.0f,
                                           x - track.getX() - thumbWidth * 0.55f, 10.0f);
    if (leftGlow.getWidth() > 2.0f)
    {
        g.setColour(Colours::goldGlow.withAlpha(0.22f));
        g.fillRoundedRectangle(leftGlow, 5.0f);
    }

    auto rightGlow = juce::Rectangle<float>(x + thumbWidth * 0.55f, track.getCentreY() - 5.0f,
                                            track.getRight() - x - thumbWidth * 0.55f, 10.0f);
    if (rightGlow.getWidth() > 2.0f)
    {
        g.setColour(Colours::blueGlow.withAlpha(0.18f));
        g.fillRoundedRectangle(rightGlow, 5.0f);
    }

    g.setColour(juce::Colours::black.withAlpha(0.56f));
    g.fillRoundedRectangle(thumb.translated(0.0f, 2.0f), 8.0f);

    juce::ColourGradient thumbBody(juce::Colour(0xff343a42), thumb.getCentreX(), thumb.getY(),
                                   juce::Colour(0xff05070a), thumb.getCentreX(), thumb.getBottom(), false);
    thumbBody.addColour(0.40, juce::Colour(0xff474f58));
    g.setGradientFill(thumbBody);
    g.fillRoundedRectangle(thumb, 8.0f);

    auto highlight = thumb;
    highlight.removeFromBottom(thumb.getHeight() * 0.60f);
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    g.fillRoundedRectangle(highlight, 8.0f);

    const float grooveX = thumb.getCentreX();
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawLine(grooveX + 0.5f, thumb.getY() + 6.0f, grooveX + 0.5f, thumb.getBottom() - 6.0f, 1.2f);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawLine(grooveX - 0.5f, thumb.getY() + 6.0f, grooveX - 0.5f, thumb.getBottom() - 6.0f, 0.9f);
}

void drawArcText(juce::Graphics& g, float cx, float cy, float radius,
                 const juce::String& text, float startDeg, float endDeg,
                 bool invertRotation, juce::Font font, juce::Colour colour)
{
    const int n = text.length();
    if (n == 0) return;

    const float startRad = juce::degreesToRadians(startDeg);
    const float endRad   = juce::degreesToRadians(endDeg);
    const float step     = (endRad - startRad) / static_cast<float>(juce::jmax(1, n - 1));

    g.setColour(colour);

    for (int i = 0; i < n; ++i)
    {
        const auto ch = juce::String::charToString(text[i]);
        const float a = startRad + step * static_cast<float>(i);
        const float x = cx + std::cos(a) * radius;
        const float y = cy + std::sin(a) * radius;
        const float rot = invertRotation
            ? a - juce::MathConstants<float>::halfPi
            : a + juce::MathConstants<float>::halfPi;

        const float w = font.getStringWidthFloat(ch);

        juce::GlyphArrangement ga;
        ga.addLineOfText(font, ch, -w * 0.5f, 0.0f);
        ga.draw(g, juce::AffineTransform::rotation(rot).translated(x, y));
    }
}

void drawSoundDeviceStamp(juce::Graphics& g, juce::Rectangle<float> area)
{
    // Mask the baked-in VERTIGO artwork with a gradient matching the top-bar metal.
    {
        juce::ColourGradient bezel(juce::Colour(0xff2a2e34), area.getX(), area.getY(),
                                   juce::Colour(0xff141619), area.getX(), area.getBottom(), false);
        bezel.addColour(0.55, juce::Colour(0xff1c1f24));
        g.setGradientFill(bezel);
        g.fillRect(area);
    }

    const auto gold = Colours::goldBright;
    const auto goldDim = Colours::gold.withAlpha(0.85f);

    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float r  = juce::jmin(area.getHeight() * 0.48f, area.getWidth() * 0.22f);

    // Outer ring.
    g.setColour(gold.withAlpha(0.95f));
    g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, juce::jmax(1.1f, r * 0.05f));

    // Inner ring.
    const float r2 = r * 0.80f;
    g.setColour(goldDim);
    g.drawEllipse(cx - r2, cy - r2, r2 * 2.0f, r2 * 2.0f, juce::jmax(0.7f, r * 0.03f));

    // Diagonal hatching clipped to inner disc.
    {
        juce::Graphics::ScopedSaveState ss(g);
        juce::Path inner;
        inner.addEllipse(cx - r2, cy - r2, r2 * 2.0f, r2 * 2.0f);
        g.reduceClipRegion(inner);

        g.setColour(gold.withAlpha(0.42f));
        const float spacing   = juce::jmax(3.5f, r * 0.18f);
        const float thickness = juce::jmax(0.8f, r * 0.05f);
        for (float offset = -r2 * 2.0f; offset <= r2 * 2.0f; offset += spacing)
        {
            g.drawLine(cx + offset - r2, cy - r2,
                       cx + offset + r2, cy + r2, thickness);
        }
    }

    // Arc text — top: "FROM A REAL STUDIO" (upper half, reading L→R).
    {
        const float arcRadius = r * 0.92f;
        const auto arcFont = VoidLookAndFeel::getSansFont(juce::jmax(6.0f, r * 0.22f), true);
        drawArcText(g, cx, cy, arcRadius, "FROM A REAL STUDIO",
                    225.0f, 315.0f, false, arcFont, gold.withAlpha(0.95f));
    }

    // Arc text — bottom: "FOR REAL STUDIOS" (lower half, reading L→R, upright).
    {
        const float arcRadius = r * 0.92f;
        const auto arcFont = VoidLookAndFeel::getSansFont(juce::jmax(6.0f, r * 0.22f), true);
        drawArcText(g, cx, cy, arcRadius, "FOR REAL STUDIOS",
                    135.0f, 45.0f, true, arcFont, gold.withAlpha(0.95f));
    }

    // Middle banner — swallowtail shape, gold fill, dark text.
    const float ribbonH   = r * 0.62f;
    const float overhang  = r * 0.30f;
    const float notch     = r * 0.16f;
    auto ribbon = juce::Rectangle<float>(cx - r - overhang, cy - ribbonH * 0.5f,
                                         (r + overhang) * 2.0f, ribbonH);

    juce::Path banner;
    banner.addRectangle(ribbon);
    banner.addTriangle(ribbon.getX(), ribbon.getY(),
                       ribbon.getX() - notch, cy,
                       ribbon.getX(), ribbon.getBottom());
    banner.addTriangle(ribbon.getRight(), ribbon.getY(),
                       ribbon.getRight() + notch, cy,
                       ribbon.getRight(), ribbon.getBottom());

    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillPath(banner, juce::AffineTransform::translation(0.0f, 1.5f));
    g.setColour(gold);
    g.fillPath(banner);

    // Thin outline on the banner for definition.
    g.setColour(juce::Colour(0xff4a3a14).withAlpha(0.85f));
    g.strokePath(banner, juce::PathStrokeType(0.8f));

    // Banner text.
    auto topHalf    = ribbon.removeFromTop(ribbonH * 0.60f);
    auto bottomHalf = ribbon;

    g.setColour(juce::Colour(0xff120d02));
    g.setFont(VoidLookAndFeel::getSansFont(topHalf.getHeight() * 0.95f, true));
    g.drawText("SOUNDEVICE", topHalf.reduced(3.0f, 0.0f), juce::Justification::centred);
    g.setFont(VoidLookAndFeel::getSansFont(bottomHalf.getHeight() * 0.82f, true));
    g.drawText("DIGITAL", bottomHalf.reduced(3.0f, 0.0f), juce::Justification::centred);
}

void drawMainLogo(juce::Graphics& g, juce::Rectangle<float> area)
{
    // Mask only the old badge footprint so the title strip remains untouched.
    {
        juce::ColourGradient bezel(juce::Colour(0xff2a2e34), area.getX(), area.getY(),
                                   juce::Colour(0xff141619), area.getX(), area.getBottom(), false);
        bezel.addColour(0.55, juce::Colour(0xff1c1f24));
        g.setGradientFill(bezel);
        g.fillRect(area);

        g.setColour(juce::Colours::white.withAlpha(0.03f));
        const float top = area.getY() + 1.0f;
        for (int i = 0; i < 4; ++i)
            g.drawHorizontalLine(static_cast<int>(std::round(top + static_cast<float>(i) * 3.0f)),
                                 area.getX(), area.getRight());
    }

    const auto gold = Colours::goldBright;
    const auto goldDim = Colours::gold.withAlpha(0.88f);
    const auto goldShadow = Colours::goldShadow.withAlpha(0.85f);

    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float r = juce::jmin(area.getHeight() * 0.47f, area.getWidth() * 0.24f);

    g.setColour(Colours::goldGlow.withAlpha(0.16f));
    g.drawEllipse(cx - r - 2.0f, cy - r - 2.0f, (r + 2.0f) * 2.0f, (r + 2.0f) * 2.0f, 2.0f);

    g.setColour(gold.withAlpha(0.95f));
    g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, juce::jmax(1.1f, r * 0.05f));

    const float r2 = r * 0.80f;
    g.setColour(goldDim);
    g.drawEllipse(cx - r2, cy - r2, r2 * 2.0f, r2 * 2.0f, juce::jmax(0.7f, r * 0.03f));

    {
        juce::Graphics::ScopedSaveState ss(g);
        juce::Path inner;
        inner.addEllipse(cx - r2, cy - r2, r2 * 2.0f, r2 * 2.0f);
        g.reduceClipRegion(inner);

        g.setColour(gold.withAlpha(0.42f));
        const float spacing = juce::jmax(3.5f, r * 0.18f);
        const float thickness = juce::jmax(0.8f, r * 0.05f);
        for (float offset = -r2 * 2.0f; offset <= r2 * 2.0f; offset += spacing)
        {
            g.drawLine(cx + offset - r2, cy - r2,
                       cx + offset + r2, cy + r2, thickness);
        }
    }

    {
        auto arcFont = VoidLookAndFeel::getSansFont(juce::jmax(5.8f, r * 0.19f), true);
        arcFont = arcFont.withHorizontalScale(0.84f);
        drawArcText(g, cx, cy, r * 0.92f, "FROM A REAL STUDIO",
                    225.0f, 315.0f, false, arcFont, gold.withAlpha(0.95f));
    }

    {
        auto arcFont = VoidLookAndFeel::getSansFont(juce::jmax(5.8f, r * 0.19f), true);
        arcFont = arcFont.withHorizontalScale(0.84f);
        drawArcText(g, cx, cy, r * 0.92f, "FOR REAL STUDIOS",
                    135.0f, 45.0f, true, arcFont, gold.withAlpha(0.95f));
    }

    g.setColour(gold.withAlpha(0.90f));
    const float bulletRadius = juce::jmax(1.2f, r * 0.06f);
    g.fillEllipse(cx - r * 0.82f - bulletRadius, cy - bulletRadius, bulletRadius * 2.0f, bulletRadius * 2.0f);
    g.fillEllipse(cx + r * 0.82f - bulletRadius, cy - bulletRadius, bulletRadius * 2.0f, bulletRadius * 2.0f);

    const float ribbonH = r * 0.64f;
    const float overhang = r * 0.36f;
    const float notch = r * 0.18f;
    auto ribbon = juce::Rectangle<float>(cx - r - overhang, cy - ribbonH * 0.5f,
                                         (r + overhang) * 2.0f, ribbonH);

    juce::Path banner;
    banner.addRectangle(ribbon);
    banner.addTriangle(ribbon.getX(), ribbon.getY(),
                       ribbon.getX() - notch, cy,
                       ribbon.getX(), ribbon.getBottom());
    banner.addTriangle(ribbon.getRight(), ribbon.getY(),
                       ribbon.getRight() + notch, cy,
                       ribbon.getRight(), ribbon.getBottom());

    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillPath(banner, juce::AffineTransform::translation(0.0f, 1.5f));

    juce::ColourGradient bannerFill(juce::Colour(0xff262126), ribbon.getCentreX(), ribbon.getY(),
                                    juce::Colour(0xff0a090b), ribbon.getCentreX(), ribbon.getBottom(), false);
    bannerFill.addColour(0.42, juce::Colour(0xff332d34));
    g.setGradientFill(bannerFill);
    g.fillPath(banner);

    g.setColour(gold.withAlpha(0.88f));
    g.strokePath(banner, juce::PathStrokeType(juce::jmax(0.9f, r * 0.035f)));

    auto bannerHighlight = ribbon.reduced(r * 0.12f, r * 0.08f);
    bannerHighlight.removeFromBottom(bannerHighlight.getHeight() * 0.58f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(bannerHighlight, r * 0.08f);

    auto topHalf = ribbon.removeFromTop(ribbonH * 0.60f);
    auto bottomHalf = ribbon;

    g.setColour(goldShadow);
    g.setFont(VoidLookAndFeel::getSansFont(topHalf.getHeight() * 0.92f, true).withHorizontalScale(0.84f));
    g.drawText("SOUNDDEVICE", topHalf.translated(0.0f, 1.0f).reduced(2.0f, 0.0f), juce::Justification::centred);
    g.setColour(gold.withAlpha(0.97f));
    g.drawText("SOUNDDEVICE", topHalf.reduced(2.0f, 0.0f), juce::Justification::centred);

    g.setColour(goldShadow);
    g.setFont(VoidLookAndFeel::getSansFont(bottomHalf.getHeight() * 0.78f, true).withHorizontalScale(0.80f));
    g.drawText("DIGITAL", bottomHalf.translated(0.0f, 1.0f).reduced(3.0f, 0.0f), juce::Justification::centred);
    g.setColour(gold.withAlpha(0.96f));
    g.drawText("DIGITAL", bottomHalf.reduced(3.0f, 0.0f), juce::Justification::centred);
}

void drawReferenceResonanceEntry(juce::Graphics& g, juce::Rectangle<float> area, bool hovered)
{
    auto labelArea = area.translated(-30.0f, -28.0f).withSizeKeepingCentre(110.0f, 20.0f);
    g.setFont(VoidLookAndFeel::getSansFont(13.0f, true));
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawText("Resonance", labelArea.translated(0.0f, 1.0f), juce::Justification::centred);
    g.setColour((hovered ? Colours::goldBright : Colours::panelText).withAlpha(hovered ? 0.95f : 0.70f));
    g.drawText("Resonance", labelArea, juce::Justification::centred);

    g.setColour((hovered ? Colours::goldBright : Colours::gold).withAlpha(hovered ? 0.65f : 0.34f));
    g.drawLine(labelArea.getCentreX(), labelArea.getBottom() + 1.0f,
               area.getCentreX(), area.getY() + 4.0f, hovered ? 1.8f : 1.0f);
}

juce::Rectangle<int> constrainEditorBounds(juce::Rectangle<int> bounds,
                                           juce::ComponentBoundsConstrainer& constrainer)
{
    const auto referenceBounds = getDefaultEditorBounds();
    constrainer.checkBounds(bounds, referenceBounds, {}, false, false, false, false);
    return bounds;
}

juce::Rectangle<int> scaleRect(int x, int y, int w, int h, juce::Component& editor)
{
    const float sx = static_cast<float>(editor.getWidth()) / static_cast<float>(kReferenceWidth);
    const float sy = static_cast<float>(editor.getHeight()) / static_cast<float>(kReferenceHeight);

    return {
        static_cast<int>(std::round(static_cast<float>(x) * sx)),
        static_cast<int>(std::round(static_cast<float>(y) * sy)),
        static_cast<int>(std::round(static_cast<float>(w) * sx)),
        static_cast<int>(std::round(static_cast<float>(h) * sy))
    };
}

void drawBypassIndicator(juce::Graphics& g, juce::Rectangle<float> area,
                         bool active, bool hovered)
{
    g.setColour(juce::Colour(0xff1d2229).withAlpha(0.97f));
    g.fillRoundedRectangle(area, 12.0f);

    juce::ColourGradient patch(juce::Colour(0xff262c33), area.getX(), area.getY(),
                               juce::Colour(0xff11151a), area.getRight(), area.getBottom(), false);
    g.setGradientFill(patch);
    g.fillRoundedRectangle(area, 12.0f);

    const auto ledBounds = juce::Rectangle<float>(area.getX() + 18.0f, area.getCentreY() - 6.0f, 12.0f, 12.0f);
    const auto ledColour = active ? juce::Colour(0xff6cff57) : juce::Colour(0xff22301d);
    g.setColour(ledColour.withAlpha(active ? 0.28f : 0.18f));
    g.fillEllipse(ledBounds.expanded(8.0f));
    g.setColour(ledColour.withAlpha(active ? 0.98f : 0.55f));
    g.fillEllipse(ledBounds);

    auto button = juce::Rectangle<float>(area.getRight() - 40.0f, area.getY() + 4.0f, 30.0f, 30.0f);
    juce::ColourGradient rim(juce::Colour(0xffe6d59a), button.getCentreX(), button.getY(),
                             juce::Colour(0xff7a6333), button.getCentreX(), button.getBottom(), false);
    rim.addColour(0.45, juce::Colour(0xffc6aa63));
    g.setGradientFill(rim);
    g.fillEllipse(button);

    g.setColour(juce::Colours::black.withAlpha(0.30f));
    g.fillEllipse(button.reduced(3.0f));

    auto inner = button.reduced(3.0f);
    juce::ColourGradient center(active ? juce::Colour(0xfff1dea4) : juce::Colour(0xffcab077),
                                inner.getCentreX() - 2.0f, inner.getY(),
                                juce::Colour(0xff70572d), inner.getCentreX() + 2.0f, inner.getBottom(), false);
    g.setGradientFill(center);
    g.fillEllipse(inner);

    auto topHalf = inner;
    topHalf.removeFromBottom(inner.getHeight() * 0.55f);
    g.setColour(juce::Colours::white.withAlpha(hovered ? 0.20f : 0.10f));
    g.fillEllipse(topHalf);
}

// Style the face-toggle + top-strip buttons consistently: gold fill when on,
// muted frame when off. Applied to any button used as a latching indicator.
void styleLatchButton(juce::TextButton& b)
{
    b.setClickingTogglesState(true);
    b.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff1b1f25));
    b.setColour(juce::TextButton::buttonOnColourId, Colours::gold.withAlpha(0.85f));
    b.setColour(juce::TextButton::textColourOffId,  Colours::panelText.withAlpha(0.80f));
    b.setColour(juce::TextButton::textColourOnId,   juce::Colour(0xff14181d));
}

void styleMomentaryLabel(juce::Label& l)
{
    l.setColour(juce::Label::textColourId, Colours::panelText.withAlpha(0.85f));
    l.setJustificationType(juce::Justification::centred);
    l.setFont(VoidLookAndFeel::getSansFont(13.0f, false));
}

} // namespace

// ---------------------------------------------------------------------------
// BandMarkerOverlay — 6 draggable/clickable markers painted on top of the
// DotMatrixDisplay. Marker position is driven by the band's freq (X, log scale)
// and gain (Y, linear in dB around 0). Click → opens the per-band popup.
// Drag (future) will drive freq/gain; for the first cut, click-only.
// ---------------------------------------------------------------------------
class BandMarkerOverlay : public juce::Component,
                          private juce::Timer
{
public:
    BandMarkerOverlay(TonalizerEditor& owner, juce::AudioProcessorValueTreeState& apvts)
        : owner_(owner), apvts_(apvts)
    {
        setInterceptsMouseClicks(true, true);
        startTimerHz(15);
    }

    void paint(juce::Graphics& g) override
    {
        const auto plot = getLocalBounds().toFloat().reduced(10.0f, 10.0f);
        for (int i = 0; i < 6; ++i)
        {
            const float freq = getParam(ParamIDs::bandFreq(i));
            const float gain = getParam(ParamIDs::bandGain(i));
            const bool  on   = getParam(ParamIDs::bandOn(i)) > 0.5f;

            const auto pos = markerPosition(freq, gain, plot);
            const float r  = on ? 9.0f : 6.5f;
            const auto col = on ? Colours::goldBright : Colours::dimText;

            g.setColour(col.withAlpha(0.25f));
            g.fillEllipse(pos.x - r - 4.0f, pos.y - r - 4.0f, (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f);
            g.setColour(col);
            g.fillEllipse(pos.x - r, pos.y - r, r * 2.0f, r * 2.0f);
            g.setColour(juce::Colour(0xff14181d));
            g.drawEllipse(pos.x - r, pos.y - r, r * 2.0f, r * 2.0f, 1.2f);

            g.setColour(col.withAlpha(0.95f));
            g.setFont(VoidLookAndFeel::getSansFont(12.0f, true));
            g.drawText(juce::String(i + 1),
                       juce::Rectangle<float>(pos.x - r, pos.y - r, r * 2.0f, r * 2.0f),
                       juce::Justification::centred);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int hit = hitTest(e.position);
        if (hit >= 0)
            owner_.openBandPopup(hit);
    }

private:
    void timerCallback() override { repaint(); }

    float getParam(const juce::String& id) const
    {
        if (auto* p = apvts_.getRawParameterValue(id))
            return p->load();
        return 0.0f;
    }

    juce::Point<float> markerPosition(float freq, float gainDb, juce::Rectangle<float> plot) const
    {
        const float logMin = std::log10(kMinFrequency);
        const float logMax = std::log10(kMaxFrequency);
        const float t = (std::log10(juce::jlimit(kMinFrequency, kMaxFrequency, freq)) - logMin) / (logMax - logMin);
        const float x = plot.getX() + plot.getWidth() * juce::jlimit(0.0f, 1.0f, t);

        const float norm = juce::jlimit(0.0f, 1.0f,
            (kMarkerMaxDb - juce::jlimit(kMarkerMinDb, kMarkerMaxDb, gainDb))
            / (kMarkerMaxDb - kMarkerMinDb));
        const float y = plot.getY() + plot.getHeight() * norm;

        return { x, y };
    }

    int hitTest(juce::Point<float> p) const
    {
        const auto plot = getLocalBounds().toFloat().reduced(10.0f, 10.0f);
        int best = -1;
        float bestDist = 18.0f * 18.0f;
        for (int i = 0; i < 6; ++i)
        {
            const auto mp = markerPosition(getParam(ParamIDs::bandFreq(i)),
                                           getParam(ParamIDs::bandGain(i)),
                                           plot);
            const float d = p.getDistanceSquaredFrom(mp);
            if (d < bestDist)
            {
                bestDist = d;
                best = i;
            }
        }
        return best;
    }

    TonalizerEditor& owner_;
    juce::AudioProcessorValueTreeState& apvts_;
};

// ---------------------------------------------------------------------------
// Per-band popup — opened by clicking a marker. Hosts freq/Q/gain/on controls
// in a small panel backed by a CallOutBox. Owns its own attachments.
// ---------------------------------------------------------------------------
namespace {

class BandPopup : public juce::Component
{
public:
    BandPopup(juce::AudioProcessorValueTreeState& apvts, int index)
    {
        title_.setText("Band " + juce::String(index + 1), juce::dontSendNotification);
        title_.setJustificationType(juce::Justification::centred);
        title_.setFont(VoidLookAndFeel::getSansFont(15.0f, true));
        title_.setColour(juce::Label::textColourId, Colours::goldBright);
        addAndMakeVisible(title_);

        auto configureSlider = [this](juce::Slider& s)
        {
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
            s.setRotaryParameters(juce::degreesToRadians(225.0f),
                                  juce::degreesToRadians(495.0f), true);
            s.setColour(juce::Slider::rotarySliderFillColourId, Colours::gold);
            s.setColour(juce::Slider::rotarySliderOutlineColourId, Colours::widgetBlack);
            s.setColour(juce::Slider::textBoxTextColourId, Colours::panelText);
            s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            addAndMakeVisible(s);
        };

        configureSlider(freqSlider_);
        configureSlider(qSlider_);
        configureSlider(gainSlider_);

        freqLabel_.setText("Freq", juce::dontSendNotification);
        qLabel_.setText("Q", juce::dontSendNotification);
        gainLabel_.setText("Gain", juce::dontSendNotification);
        for (auto* l : { &freqLabel_, &qLabel_, &gainLabel_ })
        {
            styleMomentaryLabel(*l);
            addAndMakeVisible(*l);
        }

        onBtn_.setButtonText("On");
        styleLatchButton(onBtn_);
        addAndMakeVisible(onBtn_);

        freqAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParamIDs::bandFreq(index), freqSlider_);
        qAtt_    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParamIDs::bandQ(index), qSlider_);
        gainAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParamIDs::bandGain(index), gainSlider_);
        onAtt_   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParamIDs::bandOn(index), onBtn_);

        setSize(260, 210);
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff181b20).withAlpha(0.98f));
        g.fillRoundedRectangle(b, 8.0f);
        g.setColour(Colours::goldShadow.withAlpha(0.6f));
        g.drawRoundedRectangle(b.reduced(0.5f), 8.0f, 1.0f);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(10);
        title_.setBounds(b.removeFromTop(22));
        b.removeFromTop(4);

        auto onRow = b.removeFromBottom(28);
        onBtn_.setBounds(onRow.withSizeKeepingCentre(80, 24));
        b.removeFromBottom(6);

        const int slotW = b.getWidth() / 3;
        auto freqCol = b.removeFromLeft(slotW);
        auto qCol    = b.removeFromLeft(slotW);
        auto gainCol = b;

        auto layoutSlot = [](juce::Rectangle<int> area, juce::Label& lbl, juce::Slider& sl)
        {
            lbl.setBounds(area.removeFromTop(16));
            sl.setBounds(area);
        };
        layoutSlot(freqCol, freqLabel_, freqSlider_);
        layoutSlot(qCol,    qLabel_,    qSlider_);
        layoutSlot(gainCol, gainLabel_, gainSlider_);
    }

private:
    juce::Label title_, freqLabel_, qLabel_, gainLabel_;
    juce::Slider freqSlider_, qSlider_, gainSlider_;
    juce::TextButton onBtn_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt_;
};

} // namespace

// ---------------------------------------------------------------------------
// TonalizerEditor
// ---------------------------------------------------------------------------
TonalizerEditor::TonalizerEditor(TonalizerProcessor& proc)
    : AudioProcessorEditor(proc),
      processor_(proc),
      referenceGui_(loadReferenceGui()),
      mainLogo_(loadMainLogo()),
      dotMatrix_(proc),
      eraseKnob_("Erase", proc.apvts, ParamIDs::eraseAmount),
      sharpnessKnob_("Sharpness", proc.apvts, ParamIDs::eraseSharpness),
      dryWetKnob_("Mix", proc.apvts, ParamIDs::dryWet),
      retuneKnob_("Retune", proc.apvts, ParamIDs::retuneSpeed),
      correctionKnob_("Correct", proc.apvts, ParamIDs::correctionAmount),
      keyScaleSelector_(proc),
      depthKnob_      ("Depth",       proc.apvts, ParamIDs::depth),
      thresholdKnob_  ("Threshold",   proc.apvts, ParamIDs::threshold),
      ratioKnob_      ("Ratio",       proc.apvts, ParamIDs::ratio),
      kneeKnob_       ("Knee",        proc.apvts, ParamIDs::knee),
      selectivityKnob_("Selectivity", proc.apvts, ParamIDs::selectivity),
      softHardKnob_   ("Soft/Hard",   proc.apvts, ParamIDs::softHard),
      attackKnob_     ("Attack",      proc.apvts, ParamIDs::attackMs),
      releaseKnob_    ("Release",     proc.apvts, ParamIDs::releaseMs),
      crossfeedKnob_  ("Crossfeed",   proc.apvts, ParamIDs::crossfeed)
{
    setLookAndFeel(&lookAndFeel_);

    constexpr double aspectRatio = static_cast<double>(kReferenceWidth) / static_cast<double>(kReferenceHeight);
    constrainer_.setFixedAspectRatio(aspectRatio);
    constrainer_.setMinimumSize(juce::roundToInt(static_cast<float>(kReferenceWidth) * kMinimumScale),
                                juce::roundToInt(static_cast<float>(kReferenceHeight) * kMinimumScale));
    constrainer_.setMaximumSize(juce::roundToInt(static_cast<float>(kReferenceWidth) * kMaximumScale),
                                juce::roundToInt(static_cast<float>(kReferenceHeight) * kMaximumScale));
    setConstrainer(&constrainer_);
    setResizable(true, true);

    auto editorBounds = getDefaultEditorBounds();
    const auto storedSize = proc.getStoredEditorSize();

    if (storedSize.x > 0 && storedSize.y > 0 && !isLegacyEditorSize(storedSize))
        editorBounds.setSize(storedSize.x, storedSize.y);

    editorBounds = constrainEditorBounds(editorBounds, constrainer_);
    setSize(editorBounds.getWidth(), editorBounds.getHeight());

    // Face toggle buttons.
    tuneFaceBtn_.setButtonText("Tune");
    resonanceFaceBtn_.setButtonText("Resonance");
    styleLatchButton(tuneFaceBtn_);
    styleLatchButton(resonanceFaceBtn_);
    tuneFaceBtn_.setRadioGroupId(1);
    resonanceFaceBtn_.setRadioGroupId(1);
    tuneFaceBtn_.setToggleState(true, juce::dontSendNotification);
    tuneFaceBtn_.onClick       = [this] { setFace(Face::Tune); };
    resonanceFaceBtn_.onClick  = [this] { setFace(Face::Resonance); };
    resonanceFaceBtn_.setTooltip("Open resonance controls");
    resonanceFaceBtn_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(tuneFaceBtn_);
    addAndMakeVisible(resonanceFaceBtn_);

    // Bypass (shared).
    bypassButton_.setButtonText({});
    bypassButton_.setClickingTogglesState(true);
    bypassButton_.setAlpha(0.0f);
    bypassButton_.onClick = [this] { repaint(bypassIndicatorBounds_); };
    bypassButton_.onStateChange = [this] { repaint(bypassIndicatorBounds_); };
    addAndMakeVisible(bypassButton_);
    bypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, ParamIDs::bypass, bypassButton_);

    // Shared spectrogram.
    addAndMakeVisible(dotMatrix_);

    // Tune-face children.
    crossfadeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    crossfadeSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(crossfadeSlider_);
    crossfadeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, ParamIDs::crossfade, crossfadeSlider_);

    addAndMakeVisible(eraseKnob_);
    addAndMakeVisible(sharpnessKnob_);
    addAndMakeVisible(dryWetKnob_);
    addAndMakeVisible(retuneKnob_);
    addAndMakeVisible(correctionKnob_);
    addAndMakeVisible(keyScaleSelector_);

    for (auto* k : { &eraseKnob_, &sharpnessKnob_, &dryWetKnob_, &retuneKnob_, &correctionKnob_ })
    {
        k->setArcColour(Colours::gold);
        k->setFrameVisible(true);
        TonalizerLookAndFeel::setControlVisualsVisible(k->getSlider(), false);
        k->getSlider().setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        k->getSlider().onValueChange = [this] { repaint(); };
    }

    TonalizerLookAndFeel::setControlVisualsVisible(crossfadeSlider_, false);
    crossfadeSlider_.setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    crossfadeSlider_.onValueChange = [this] { repaint(); };

    // Tune face labels.
    struct TuneLabel { juce::Label* label; const char* text; };
    const TuneLabel tuneLabels[] = {
        { &eraseLabel_,      "Erase"     },
        { &sharpnessLabel_,  "Sharpness" },
        { &dryWetLabel_,     "Mix"       },
        { &crossfadeLabel_,  "Erase \xe2\x86\x94 Tune" },
        { &retuneLabel_,     "Retune"    },
        { &correctionLabel_, "Correct"   },
    };
    for (const auto& tl : tuneLabels)
    {
        tl.label->setText(tl.text, juce::dontSendNotification);
        tl.label->setFont(VoidLookAndFeel::getSansFont(13.0f, true));
        tl.label->setColour(juce::Label::textColourId, Colours::panelText.withAlpha(0.9f));
        tl.label->setJustificationType(juce::Justification::centred);
        tl.label->setAlpha(0.0f);
        addAndMakeVisible(*tl.label);
    }

    // Resonance-face children — top strip.
    deltaBtn_.setButtonText("Delta");
    sidechainBtn_.setButtonText("Sidechain");
    msBtn_.setButtonText("M/S");
    for (auto* b : { &deltaBtn_, &sidechainBtn_, &msBtn_, &stereoLinkBtn_ })
        styleLatchButton(*b);
    stereoLinkBtn_.setButtonText("Link");

    deltaAttachment_     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, ParamIDs::delta,       deltaBtn_);
    sidechainAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, ParamIDs::sidechainOn, sidechainBtn_);
    stereoLinkAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, ParamIDs::stereoLink,  stereoLinkBtn_);

    // stereoMode is a 2-choice AudioParameterChoice (0=L/R, 1=M/S). Wire the
    // toggle button manually since ComboBoxAttachment would need a ComboBox.
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(ParamIDs::stereoMode)))
    {
        msBtn_.setToggleState(p->getIndex() == 1, juce::dontSendNotification);
        msBtn_.onClick = [this, &proc]
        {
            auto* pc = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(ParamIDs::stereoMode));
            if (pc == nullptr) return;
            const int target = msBtn_.getToggleState() ? 1 : 0;
            if (pc->getIndex() != target)
            {
                pc->beginChangeGesture();
                *pc = target;
                pc->endChangeGesture();
            }
        };
        proc.apvts.addParameterListener(ParamIDs::stereoMode, this);
    }

    const juce::StringArray qualityNames {
        "Eco (1024)", "Low (2048)", "Standard (4096)", "High (8192)", "Insane (16384)"
    };
    for (int i = 0; i < qualityNames.size(); ++i)
    {
        qualityCombo_.addItem(qualityNames[i], i + 1);
        offlineQualityCombo_.addItem(qualityNames[i], i + 1);
    }
    qualityAttachment_        = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, ParamIDs::quality,        qualityCombo_);
    offlineQualityAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, ParamIDs::offlineQuality, offlineQualityCombo_);

    addAndMakeVisible(deltaBtn_);
    addAndMakeVisible(sidechainBtn_);
    addAndMakeVisible(msBtn_);
    addAndMakeVisible(qualityCombo_);
    addAndMakeVisible(offlineQualityCombo_);

    // Resonance-face children — knob rows.
    for (auto* k : { &depthKnob_, &thresholdKnob_, &ratioKnob_, &kneeKnob_, &selectivityKnob_, &softHardKnob_,
                     &attackKnob_, &releaseKnob_, &crossfeedKnob_ })
    {
        k->setArcColour(Colours::gold);
        k->setFrameVisible(true);
        addAndMakeVisible(*k);
    }
    addAndMakeVisible(stereoLinkBtn_);

    // Resonance-face labels.
    struct ResLabel { juce::Label* label; const char* text; };
    const ResLabel resLabels[] = {
        { &depthLabel_,       "Depth"       },
        { &thresholdLabel_,   "Threshold"   },
        { &ratioLabel_,       "Ratio"       },
        { &kneeLabel_,        "Knee"        },
        { &selectivityLabel_, "Selectivity" },
        { &softHardLabel_,    "Soft/Hard"   },
        { &attackLabel_,      "Attack"      },
        { &releaseLabel_,     "Release"     },
        { &crossfeedLabel_,   "Crossfeed"   },
    };
    for (const auto& rl : resLabels)
    {
        rl.label->setText(rl.text, juce::dontSendNotification);
        rl.label->setFont(VoidLookAndFeel::getSansFont(13.0f, true));
        rl.label->setColour(juce::Label::textColourId, Colours::panelText.withAlpha(0.9f));
        rl.label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*rl.label);
    }

    // Band markers overlay.
    bandOverlay_ = std::make_unique<BandMarkerOverlay>(*this, proc.apvts);
    addChildComponent(*bandOverlay_);

    updateFaceVisibility();
}

TonalizerEditor::~TonalizerEditor()
{
    processor_.apvts.removeParameterListener(ParamIDs::stereoMode, this);
    setLookAndFeel(nullptr);
}

void TonalizerEditor::paint(juce::Graphics& g)
{
    if (currentFace_ == Face::Tune && shouldDrawReferenceGui(referenceGui_))
    {
        g.drawImage(referenceGui_, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);

        const auto main_logo = scaleRect(98, 16, 128, 54, *this).toFloat();
        if (mainLogo_.isValid())
            g.drawImage(mainLogo_, main_logo, juce::RectanglePlacement::stretchToFit);
        else
            drawMainLogo(g, main_logo);

        drawReferenceBypassControl(g, bypassIndicatorBounds_.toFloat(),
                                   bypassButton_.getToggleState(),
                                   bypassButton_.isMouseOverOrDragging());

        drawReferenceKnobIndicator(g, eraseKnob_.getBounds().toFloat(),
                                   normalisedSliderValue(eraseKnob_.getSlider()), 0.06f, 0.42f);
        drawReferenceKnobIndicator(g, sharpnessKnob_.getBounds().toFloat(),
                                   normalisedSliderValue(sharpnessKnob_.getSlider()), 0.08f, 0.38f);
        drawReferenceKnobIndicator(g, dryWetKnob_.getBounds().toFloat(),
                                   normalisedSliderValue(dryWetKnob_.getSlider()), 0.08f, 0.38f);
        drawReferenceKnobIndicator(g, retuneKnob_.getBounds().toFloat(),
                                   normalisedSliderValue(retuneKnob_.getSlider()), 0.08f, 0.38f);
        drawReferenceKnobIndicator(g, correctionKnob_.getBounds().toFloat(),
                                   normalisedSliderValue(correctionKnob_.getSlider()), 0.08f, 0.38f);
        drawReferenceCrossfadeIndicator(g, crossfadeSlider_.getBounds().toFloat(),
                                        normalisedSliderValue(crossfadeSlider_));

        drawReferenceResonanceEntry(g, resonanceHotspotBounds_.toFloat(),
                                    resonanceFaceBtn_.isMouseOverOrDragging());

        if (resonanceFaceBtn_.isMouseOverOrDragging())
            drawReferenceHotspotHint(g, resonanceHotspotBounds_.toFloat());

        return;
    }

    VoidLookAndFeel::drawMetalPanel(g, getLocalBounds());

    // Title strip — no baked art, just drawn text so nothing can drift out of
    // alignment with the real controls.
    const auto titleArea = scaleRect(280, 14, 540, 44, *this);
    g.setColour(Colours::goldBright);
    g.setFont(VoidLookAndFeel::getSansFont(static_cast<float>(titleArea.getHeight()) * 0.55f, true));
    g.drawText("Tonalizer  -  RESONANCE", titleArea, juce::Justification::centred);

    drawBypassIndicator(g, bypassIndicatorBounds_.toFloat(), bypassButton_.getToggleState(),
                        bypassButton_.isMouseOverOrDragging());
}

void TonalizerEditor::resized()
{
    processor_.setStoredEditorSize({ getWidth(), getHeight() });

    // Bypass indicator and click-zone (shared across faces).
    bypassIndicatorBounds_ = scaleRect(975, 23, 60, 32, *this);
    bypassButton_.setBounds(scaleRect(904, 18, 138, 42, *this));
    resonanceHotspotBounds_ = scaleRect(1038, 650, 86, 92, *this);

    if (currentFace_ == Face::Tune)
    {
        tuneFaceBtn_.setBounds(0, 0, 0, 0);
        resonanceFaceBtn_.setBounds(resonanceHotspotBounds_);
    }
    else
    {
        tuneFaceBtn_.setBounds(scaleRect(24, 20, 90, 32, *this));
        resonanceFaceBtn_.setBounds(scaleRect(122, 20, 120, 32, *this));
    }

    // Shared spectrogram.
    dotMatrix_.setBounds(scaleRect(103, 93, 924, 314, *this));
    if (bandOverlay_ != nullptr)
        bandOverlay_->setBounds(dotMatrix_.getBounds());

    // Tune face — 5 knobs + crossfade slider across the same width as the
    // spectrogram (x=103..1027, w=924). Labels sit above each knob.
    {
        eraseLabel_.setBounds(scaleRect(88, 421, 90, 18, *this));
        sharpnessLabel_.setBounds(scaleRect(238, 442, 110, 18, *this));
        dryWetLabel_.setBounds(scaleRect(357, 442, 80, 18, *this));
        crossfadeLabel_.setBounds(scaleRect(503, 440, 280, 20, *this));
        retuneLabel_.setBounds(scaleRect(850, 441, 90, 18, *this));
        correctionLabel_.setBounds(scaleRect(960, 441, 90, 18, *this));

        eraseKnob_.setBounds(scaleRect(66, 446, 126, 126, *this));
        sharpnessKnob_.setBounds(scaleRect(244, 474, 92, 92, *this));
        dryWetKnob_.setBounds(scaleRect(356, 474, 92, 92, *this));
        crossfadeSlider_.setBounds(scaleRect(452, 462, 392, 96, *this));
        retuneKnob_.setBounds(scaleRect(852, 474, 94, 94, *this));
        correctionKnob_.setBounds(scaleRect(958, 474, 94, 94, *this));
    }

    keyScaleSelector_.setBounds(getLocalBounds());

    // Resonance top strip — right of the face toggles, left of the bypass.
    deltaBtn_.setBounds           (scaleRect(260, 20,  80, 32, *this));
    sidechainBtn_.setBounds       (scaleRect(348, 20, 110, 32, *this));
    msBtn_.setBounds              (scaleRect(466, 20,  70, 32, *this));
    qualityCombo_.setBounds       (scaleRect(554, 20, 160, 32, *this));
    offlineQualityCombo_.setBounds(scaleRect(724, 20, 160, 32, *this));

    // Resonance knob row 1 — 6 knobs evenly across the spectrogram width.
    {
        const int labelH = 16;
        const int labelY = 420;
        const int knobY  = labelY + labelH + 4;
        const int knobW  = 84, knobH = 84;
        const int totalW = 924;
        const int slotW  = totalW / 6;

        EraseKnob*   knobs[6]  = { &depthKnob_, &thresholdKnob_, &ratioKnob_,
                                   &kneeKnob_,  &selectivityKnob_, &softHardKnob_ };
        juce::Label* labels[6] = { &depthLabel_, &thresholdLabel_, &ratioLabel_,
                                   &kneeLabel_,  &selectivityLabel_, &softHardLabel_ };
        for (int i = 0; i < 6; ++i)
        {
            const int cx = 103 + slotW * i + slotW / 2;
            labels[i]->setBounds(scaleRect(cx - 60, labelY, 120, labelH, *this));
            knobs[i]->setBounds (scaleRect(cx - knobW / 2, knobY, knobW, knobH, *this));
        }
    }

    // Resonance knob row 2 — 3 knobs + stereo-link toggle.
    {
        const int labelH = 16;
        const int labelY = 540;
        const int knobY  = labelY + labelH + 4;
        const int knobW  = 84, knobH = 84;
        const int totalW = 924;
        const int slotW  = totalW / 4;

        EraseKnob*   knobs[3]  = { &attackKnob_, &releaseKnob_, &crossfeedKnob_ };
        juce::Label* labels[3] = { &attackLabel_, &releaseLabel_, &crossfeedLabel_ };
        for (int i = 0; i < 3; ++i)
        {
            const int cx = 103 + slotW * i + slotW / 2;
            labels[i]->setBounds(scaleRect(cx - 60, labelY, 120, labelH, *this));
            knobs[i]->setBounds (scaleRect(cx - knobW / 2, knobY, knobW, knobH, *this));
        }

        const int cx = 103 + slotW * 3 + slotW / 2;
        stereoLinkBtn_.setBounds(scaleRect(cx - 50, knobY + 24, 100, 36, *this));
    }
}

void TonalizerEditor::setFace(Face face)
{
    if (face == currentFace_) return;
    currentFace_ = face;
    tuneFaceBtn_.setToggleState     (face == Face::Tune,      juce::dontSendNotification);
    resonanceFaceBtn_.setToggleState(face == Face::Resonance, juce::dontSendNotification);
    updateFaceVisibility();
    resized();
    repaint();
}

void TonalizerEditor::updateFaceVisibility()
{
    const bool tune = currentFace_ == Face::Tune;
    const bool res  = ! tune;

    juce::Component* const tuneOnly[] = {
        &eraseKnob_, &sharpnessKnob_, &dryWetKnob_, &retuneKnob_, &correctionKnob_,
        &crossfadeSlider_, &keyScaleSelector_,
        &eraseLabel_, &sharpnessLabel_, &dryWetLabel_, &retuneLabel_, &correctionLabel_,
        &crossfadeLabel_,
    };
    for (auto* c : tuneOnly) c->setVisible(tune);

    juce::Component* const resOnly[] = {
        &deltaBtn_, &sidechainBtn_, &msBtn_, &qualityCombo_, &offlineQualityCombo_,
        &stereoLinkBtn_,
        &depthKnob_, &thresholdKnob_, &ratioKnob_, &kneeKnob_, &selectivityKnob_, &softHardKnob_,
        &attackKnob_, &releaseKnob_, &crossfeedKnob_,
        &depthLabel_, &thresholdLabel_, &ratioLabel_, &kneeLabel_, &selectivityLabel_, &softHardLabel_,
        &attackLabel_, &releaseLabel_, &crossfeedLabel_,
    };
    for (auto* c : resOnly) c->setVisible(res);

    if (bandOverlay_ != nullptr)
        bandOverlay_->setVisible(res);

    tuneFaceBtn_.setVisible(res);
    resonanceFaceBtn_.setVisible(true);

    if (tune)
    {
        resonanceFaceBtn_.setButtonText({});
        resonanceFaceBtn_.setAlpha(0.001f);
    }
    else
    {
        tuneFaceBtn_.setButtonText("Tune");
        tuneFaceBtn_.setAlpha(1.0f);
        resonanceFaceBtn_.setButtonText("Resonance");
        resonanceFaceBtn_.setAlpha(1.0f);
    }
}

void TonalizerEditor::parameterChanged(const juce::String& id, float /*newValue*/)
{
    // Mirror stereoMode changes back to the M/S button from whichever thread
    // the host set them on. The message thread does the actual UI update.
    if (id == ParamIDs::stereoMode)
    {
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<TonalizerEditor>(this)]
        {
            if (auto* ed = safe.getComponent())
            {
                if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(
                        ed->processor_.apvts.getParameter(ParamIDs::stereoMode)))
                {
                    ed->msBtn_.setToggleState(p->getIndex() == 1, juce::dontSendNotification);
                }
            }
        });
    }
}

void TonalizerEditor::openBandPopup(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= 6) return;

    auto popup = std::make_unique<BandPopup>(processor_.apvts, bandIndex);
    const auto screenBounds = bandOverlay_ != nullptr
        ? bandOverlay_->getScreenBounds()
        : getScreenBounds();

    auto anchor = screenBounds.withSizeKeepingCentre(1, 1);
    juce::CallOutBox::launchAsynchronously(std::move(popup), anchor, nullptr);
}

} // namespace tonalizer
