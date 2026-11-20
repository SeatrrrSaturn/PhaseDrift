#include "PhaseLookAndFeel.h"

//==============================================================================
namespace
{
    void drawTracked (juce::Graphics& g, const juce::String& text, juce::Font font,
                      juce::Colour colour, juce::Rectangle<float> area,
                      juce::Justification just, float tracking)
    {
        g.setFont (font);
        g.setColour (colour);

        const auto w = font.getStringWidthFloat (text) + tracking * (float) juce::jmax (0, text.length() - 1);

        float x = area.getX();
        if (just.testFlags (juce::Justification::horizontallyCentred))
            x = area.getCentreX() - w * 0.5f;
        else if (just.testFlags (juce::Justification::right))
            x = area.getRight() - w;

        for (int i = 0; i < text.length(); ++i)
        {
            const auto ch = juce::String::charToString (text[i]);
            g.drawText (ch, juce::Rectangle<float> (x, area.getY(), 20.0f, area.getHeight()),
                        juce::Justification::centredLeft, false);
            x += font.getStringWidthFloat (ch) + tracking;
        }
    }
}

//==============================================================================
PhaseLAF::PhaseLAF()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0e1319));
    setColour (juce::ComboBox::textColourId,       juce::Colour (0xffd8e6ef));
    setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff243040));
    setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xff3be8ff));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0e1319));
    setColour (juce::PopupMenu::textColourId,       juce::Colour (0xffc9d7e2));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff193039));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff3be8ff));
    setColour (juce::Label::textColourId, juce::Colour (0xffd8e6ef));
}

juce::Colour PhaseLAF::accentOf (const juce::Slider& s)
{
    const juce::var v = s.getProperties().getWithDefault ("accent", (int) 0xff3be8ff);
    return juce::Colour ((juce::uint32) (int) v);
}

//==============================================================================
void PhaseLAF::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                 float pos, float startA, float endA, juce::Slider& slider)
{
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const auto centre = area.getCentre();
    const float radius = size * 0.5f - 1.0f;
    const float angle  = startA + pos * (endA - startA);

    const juce::Colour accent = accentOf (slider);
    const bool bipolar = slider.getProperties().getWithDefault ("bipolar", false);
    const float zeroT  = bipolar ? 0.5f : 0.0f;

    // ---- tick ring --------------------------------------------------------
    const int numTicks = 27;
    for (int i = 0; i < numTicks; ++i)
    {
        const float t = (float) i / (float) (numTicks - 1);
        const float a = startA + t * (endA - startA);
        const float r1 = radius * 0.925f;
        const float r2 = radius * (i % 5 == 0 ? 1.00f : 0.965f);

        const bool lit = (t >= juce::jmin (zeroT, pos) - 0.001f && t <= juce::jmax (zeroT, pos) + 0.001f);
        const bool major = (i % 5 == 0);

        g.setColour (lit ? accent.withAlpha (0.9f)
                         : juce::Colour (0xff2b3440).withAlpha (major ? 0.85f : 0.55f));

        juce::Path p;
        p.startNewSubPath (centre.x + std::sin (a) * r1, centre.y - std::cos (a) * r1);
        p.lineTo          (centre.x + std::sin (a) * r2, centre.y - std::cos (a) * r2);
        g.strokePath (p, juce::PathStrokeType (lit ? 1.6f : 1.2f));
    }

    // ---- track ------------------------------------------------------------
    const float arcR = radius * 0.845f;

    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startA, endA, true);
        g.setColour (juce::Colour (0xff1b222c));
        g.strokePath (track, juce::PathStrokeType (4.2f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // ---- value arc --------------------------------------------------------
    {
        const float fromA = startA + zeroT * (endA - startA);

        juce::Path arc;
        if (angle >= fromA)
            arc.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, fromA, angle, true);
        else
            arc.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, angle, fromA, false);

        if (! arc.isEmpty())
        {
            g.setColour (accent.withAlpha (0.16f));
            g.strokePath (arc, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

            g.setColour (accent.withAlpha (0.95f));
            g.strokePath (arc, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }

        // glowing tip
        const juce::Point<float> tip (centre.x + std::sin (angle) * arcR,
                                      centre.y - std::cos (angle) * arcR);
        g.setColour (accent.withAlpha (0.35f));
        g.fillEllipse (tip.x - 5.0f, tip.y - 5.0f, 10.0f, 10.0f);
        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.fillEllipse (tip.x - 1.8f, tip.y - 1.8f, 3.6f, 3.6f);
    }

    // ---- knob body --------------------------------------------------------
    const float bodyR = radius * 0.70f;

    {
        juce::ColourGradient grad (juce::Colour (0xff39424f),
                                   centre.x - bodyR * 0.38f, centre.y - bodyR * 0.45f,
                                   juce::Colour (0xff080b0f),
                                   centre.x + bodyR * 0.48f, centre.y + bodyR * 0.52f,
                                   true);
        g.setGradientFill (grad);
        g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        // soft accent bloom under the dial
        juce::ColourGradient bloom (accent.withAlpha (0.10f), centre.x, centre.y + bodyR * 0.7f,
                                    accent.withAlpha (0.0f),  centre.x, centre.y - bodyR * 0.2f,
                                    true);
        g.setGradientFill (bloom);
        g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        g.setColour (juce::Colour (0xff05070a).withAlpha (0.85f));
        g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.2f);

        juce::Path rim;
        rim.addCentredArc (centre.x, centre.y, bodyR * 0.93f, bodyR * 0.93f, 0.0f, -2.35f, -0.85f, true);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.strokePath (rim, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        juce::Path lower;
        lower.addCentredArc (centre.x, centre.y, bodyR * 0.93f, bodyR * 0.93f, 0.0f, 0.85f, 2.3f, true);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.strokePath (lower, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // ---- pointer ----------------------------------------------------------
    {
        const float pIn  = bodyR * 0.20f;
        const float pOut = bodyR * 0.84f;

        juce::Path p;
        p.startNewSubPath (centre.x + std::sin (angle) * pIn, centre.y - std::cos (angle) * pIn);
        p.lineTo          (centre.x + std::sin (angle) * pOut, centre.y - std::cos (angle) * pOut);

        g.setColour (accent.withAlpha (0.22f));
        g.strokePath (p, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

        g.setColour (accent);
        g.strokePath (p, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillEllipse (centre.x + std::sin (angle) * pOut - 1.7f,
                       centre.y - std::cos (angle) * pOut - 1.7f, 3.4f, 3.4f);

        g.setColour (juce::Colour (0xff05070a));
        g.fillEllipse (centre.x - bodyR * 0.11f, centre.y - bodyR * 0.11f,
                       bodyR * 0.22f, bodyR * 0.22f);
    }
}

//==============================================================================
void PhaseLAF::drawComboBox (juce::Graphics& g, int width, int height, bool,
                             int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);

    g.setColour (juce::Colour (0xff0c1015));
    g.fillRoundedRectangle (r, 5.0f);

    g.setColour (juce::Colour (0xff22303d));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    // arrow
    juce::Path arrow;
    const float ax = (float) width - 16.0f;
    const float ay = (float) height * 0.5f;
    arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.5f);
    arrow.lineTo (ax + 4.0f, ay - 2.0f);
    g.setColour (juce::Colour (0xff3be8ff).withAlpha (0.85f));
    g.strokePath (arrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::ignoreUnused (box);
}

void PhaseLAF::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (juce::Font (12.5f));
    label.setColour (juce::Label::textColourId, juce::Colour (0xffd8e6ef));
}

//==============================================================================
void PhaseLAF::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                     const juce::Colour&, bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = button.getToggleState();

    juce::Colour fill = on ? juce::Colour (0xff1b3a44) : juce::Colour (0xff0d1218);
    if (down)      fill = fill.brighter (0.08f);
    else if (highlighted) fill = fill.brighter (0.04f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, 6.0f);

    g.setColour (on ? juce::Colour (0xff3be8ff).withAlpha (0.7f) : juce::Colour (0xff22303d));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    if (on)
    {
        g.setColour (juce::Colour (0xff3be8ff).withAlpha (0.16f));
        g.fillRoundedRectangle (r, 6.0f);
    }
}

void PhaseLAF::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (0xff0c1015));
    g.setColour (juce::Colour (0xff22303d));
    g.drawRect (0, 0, width, height, 1);
}

//==============================================================================
LabelledKnob::LabelledKnob (const juce::String& name, juce::Colour accent, bool bipolar)
    : label (name), accentColour (accent)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f,
                                true);
    slider.getProperties().set ("accent", (int) accent.getARGB());
    slider.getProperties().set ("bipolar", bipolar);
    slider.setMouseDragSensitivity (220);
    slider.setVelocityBasedMode (false);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
}

void LabelledKnob::setCompact (bool shouldBeCompact)
{
    compact = shouldBeCompact;
    repaint();
}

void LabelledKnob::resized()
{
    auto b = getLocalBounds();
    b.removeFromTop (compact ? 12 : 13);
    b.removeFromBottom (compact ? 12 : 13);

    const int side = juce::jmin (b.getWidth(), b.getHeight());
    slider.setBounds (b.withSizeKeepingCentre (side, side));
}

void LabelledKnob::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    drawTracked (g, label.toUpperCase(),
                 juce::Font (compact ? 8.0f : 8.6f),
                 juce::Colour (0xff67788a),
                 b.removeFromTop (compact ? 12.0f : 13.0f),
                 juce::Justification::centred, 0.9f);

    const juce::String valueText = slider.getTextFromValue (slider.getValue());

    drawTracked (g, valueText,
                 juce::Font (compact ? 9.0f : 9.6f),
                 accentColour.withAlpha (0.92f),
                 b.removeFromBottom (compact ? 12.0f : 13.0f),
                 juce::Justification::centred, 0.3f);
}
