#include "PluginEditor.h"

//==============================================================================
namespace
{
    const juce::Colour kBg      { 0xff080a0d };
    const juce::Colour kBgDeep  { 0xff030508 };
    const juce::Colour kPanel   { 0xff0c1015 };
    const juce::Colour kBorder  { 0xff1a2430 };
    const juce::Colour kDim     { 0xff5f7183 };
    const juce::Colour kText    { 0xffe2edf3 };
    const juce::Colour kCyan    { 0xff3be8ff };
    const juce::Colour kPink    { 0xffff3d8a };
    const juce::Colour kLime    { 0xffb4ff39 };
    const juce::Colour kViolet  { 0xff9d6bff };
    const juce::Colour kAmber   { 0xffffb454 };

    const juce::Colour kBandAccents[pid::numBands] { kCyan, kPink, kLime, kViolet };
    const char* const kBandNames[pid::numBands]  { "LOW", "LOW MID", "HIGH MID", "AIR" };

    void drawTracked (juce::Graphics& g, const juce::String& text, juce::Font font,
                      juce::Colour colour, juce::Rectangle<float> area,
                      juce::Justification just, float tracking)
    {
        g.setFont (font);
        g.setColour (colour);

        const float w = font.getStringWidthFloat (text)
                        + tracking * (float) juce::jmax (0, text.length() - 1);

        float x = area.getX();
        if (just.testFlags (juce::Justification::horizontallyCentred))
            x = area.getCentreX() - w * 0.5f;
        else if (just.testFlags (juce::Justification::right))
            x = area.getRight() - w;

        for (int i = 0; i < text.length(); ++i)
        {
            const auto ch = juce::String::charToString (text[i]);
            g.drawText (ch, juce::Rectangle<float> (x, area.getY(), 24.0f, area.getHeight()),
                        juce::Justification::centredLeft, false);
            x += font.getStringWidthFloat (ch) + tracking;
        }
    }

    // Preset definitions -----------------------------------------------------
    struct PresetValue { const char* id; float value; };

    struct Preset
    {
        const char* name;
        const PresetValue* values;
        int count;
    };

    const PresetValue initValues[] {
        { pid::inGain, 0.0f }, { pid::mix, 1.0f }, { pid::outGain, 0.0f },
        { pid::drive, 0.12f }, { pid::ring, 0.0f }, { pid::tilt, 0.0f },
        { pid::feedback, 0.35f }, { pid::fbTone, 0.55f }, { pid::chaos, 0.15f },
        { pid::topology, 0.2f }, { pid::spread, 0.35f }, { pid::msRotate, 0.0f },
        { pid::lfoSpeed, 1.0f }, { pid::lfoWave, 0.3f }, { pid::lfoPhase, 0.0f },
        { pid::bandFreq (0), 90.0f },  { pid::bandPhase (0), 0.35f },
        { pid::bandDepth (0), 0.22f }, { pid::bandRate (0), 0.17f },
        { pid::bandFreq (1), 720.0f }, { pid::bandPhase (1), -0.2f },
        { pid::bandDepth (1), 0.18f }, { pid::bandRate (1), 0.43f },
        { pid::bandFreq (2), 3100.0f }, { pid::bandPhase (2), 0.5f },
        { pid::bandDepth (2), 0.26f }, { pid::bandRate (2), 0.71f },
        { pid::bandFreq (3), 9500.0f }, { pid::bandPhase (3), 0.15f },
        { pid::bandDepth (3), 0.12f }, { pid::bandRate (3), 1.13f }
    };

    const PresetValue glassValues[] {
        { pid::inGain, -3.0f }, { pid::mix, 1.0f }, { pid::outGain, 1.0f },
        { pid::drive, 0.22f }, { pid::ring, 0.05f }, { pid::tilt, 0.15f },
        { pid::feedback, 0.46f }, { pid::fbTone, 0.8f }, { pid::chaos, 0.1f },
        { pid::topology, 0.62f }, { pid::spread, 0.62f }, { pid::msRotate, 12.0f },
        { pid::lfoSpeed, 0.75f }, { pid::lfoWave, 0.34f }, { pid::lfoPhase, 40.0f },
        { pid::bandFreq (0), 150.0f },  { pid::bandPhase (0), 0.62f },
        { pid::bandDepth (0), 0.38f },  { pid::bandRate (0), 0.11f },
        { pid::bandFreq (1), 1250.0f }, { pid::bandPhase (1), -0.42f },
        { pid::bandDepth (1), 0.32f },  { pid::bandRate (1), 0.27f },
        { pid::bandFreq (2), 5400.0f }, { pid::bandPhase (2), 0.78f },
        { pid::bandDepth (2), 0.48f },  { pid::bandRate (2), 0.55f },
        { pid::bandFreq (3), 15000.0f }, { pid::bandPhase (3), -0.24f },
        { pid::bandDepth (3), 0.22f },   { pid::bandRate (3), 0.92f }
    };

    const PresetValue ghostValues[] {
        { pid::inGain, 0.0f }, { pid::mix, 1.0f }, { pid::outGain, -1.5f },
        { pid::drive, 0.06f }, { pid::ring, 0.0f }, { pid::tilt, -0.35f },
        { pid::feedback, 0.72f }, { pid::fbTone, 0.75f }, { pid::chaos, 0.28f },
        { pid::topology, 1.0f }, { pid::spread, 0.22f }, { pid::msRotate, 0.0f },
        { pid::lfoSpeed, 0.3f }, { pid::lfoWave, 0.5f }, { pid::lfoPhase, 0.0f },
        { pid::bandFreq (0), 62.0f },  { pid::bandPhase (0), 0.86f },
        { pid::bandDepth (0), 0.5f },  { pid::bandRate (0), 0.06f },
        { pid::bandFreq (1), 480.0f }, { pid::bandPhase (1), -0.6f },
        { pid::bandDepth (1), 0.42f }, { pid::bandRate (1), 0.13f },
        { pid::bandFreq (2), 2400.0f }, { pid::bandPhase (2), 0.9f },
        { pid::bandDepth (2), 0.6f },   { pid::bandRate (2), 0.21f },
        { pid::bandFreq (3), 8200.0f }, { pid::bandPhase (3), 0.52f },
        { pid::bandDepth (3), 0.36f },  { pid::bandRate (3), 0.33f }
    };

    const PresetValue dopplerValues[] {
        { pid::inGain, 0.0f }, { pid::mix, 0.88f }, { pid::outGain, 0.0f },
        { pid::drive, 0.25f }, { pid::ring, 0.35f }, { pid::tilt, 0.0f },
        { pid::feedback, 0.32f }, { pid::fbTone, 0.4f }, { pid::chaos, 0.12f },
        { pid::topology, 0.3f }, { pid::spread, 0.85f }, { pid::msRotate, 45.0f },
        { pid::lfoSpeed, 2.2f }, { pid::lfoWave, 0.15f }, { pid::lfoPhase, 90.0f },
        { pid::bandFreq (0), 42.0f },  { pid::bandPhase (0), -0.92f },
        { pid::bandDepth (0), 0.72f }, { pid::bandRate (0), 0.05f },
        { pid::bandFreq (1), 320.0f }, { pid::bandPhase (1), 0.82f },
        { pid::bandDepth (1), 0.62f }, { pid::bandRate (1), 0.09f },
        { pid::bandFreq (2), 1900.0f }, { pid::bandPhase (2), -0.72f },
        { pid::bandDepth (2), 0.55f },  { pid::bandRate (2), 0.14f },
        { pid::bandFreq (3), 6200.0f }, { pid::bandPhase (3), 0.9f },
        { pid::bandDepth (3), 0.42f },  { pid::bandRate (3), 0.22f }
    };

    const PresetValue meltValues[] {
        { pid::inGain, -2.0f }, { pid::mix, 1.0f }, { pid::outGain, -1.0f },
        { pid::drive, 0.55f }, { pid::ring, 0.08f }, { pid::tilt, 0.2f },
        { pid::feedback, 0.6f }, { pid::fbTone, 0.3f }, { pid::chaos, 0.65f },
        { pid::topology, 0.45f }, { pid::spread, 0.45f }, { pid::msRotate, -25.0f },
        { pid::lfoSpeed, 0.55f }, { pid::lfoWave, 0.42f }, { pid::lfoPhase, 200.0f },
        { pid::bandFreq (0), 120.0f }, { pid::bandPhase (0), 0.42f },
        { pid::bandDepth (0), 0.36f }, { pid::bandRate (0), 0.35f },
        { pid::bandFreq (1), 900.0f }, { pid::bandPhase (1), -0.52f },
        { pid::bandDepth (1), 0.46f }, { pid::bandRate (1), 0.61f },
        { pid::bandFreq (2), 4100.0f }, { pid::bandPhase (2), 0.72f },
        { pid::bandDepth (2), 0.5f },   { pid::bandRate (2), 1.1f },
        { pid::bandFreq (3), 12000.0f }, { pid::bandPhase (3), 0.3f },
        { pid::bandDepth (3), 0.3f },    { pid::bandRate (3), 1.7f }
    };

    const Preset kPresets[] {
        { "INIT",          initValues,    (int) (sizeof (initValues)    / sizeof (PresetValue)) },
        { "GLASS ROTOR",   glassValues,   (int) (sizeof (glassValues)   / sizeof (PresetValue)) },
        { "GHOST COMB",    ghostValues,   (int) (sizeof (ghostValues)   / sizeof (PresetValue)) },
        { "DOPPLER WELL",  dopplerValues, (int) (sizeof (dopplerValues) / sizeof (PresetValue)) },
        { "TAPE MELT",     meltValues,    (int) (sizeof (meltValues)    / sizeof (PresetValue)) }
    };
}

//==============================================================================
PhaseDriftEditor::PhaseDriftEditor (PhaseDriftProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);
    setOpaque (true);

    // --- spectral bands ---------------------------------------------------
    for (int b = 0; b < pid::numBands; ++b)
    {
        addKnob (pid::bandFreq (b),  "FREQ",  kBandAccents[b], false, true);
        addKnob (pid::bandPhase (b), "PHASE", kBandAccents[b], true,  true);
        addKnob (pid::bandDepth (b), "DEPTH", kBandAccents[b], false, true);
        addKnob (pid::bandRate (b),  "RATE",  kBandAccents[b], false, true);
    }

    // --- core -------------------------------------------------------------
    addKnob (pid::inGain,   "IN",      kAmber,  true,  false);
    addKnob (pid::drive,    "DRIVE",   kPink,   false, false);
    addKnob (pid::tilt,     "TILT",    kViolet, true,  false);
    addKnob (pid::mix,      "MIX",     kCyan,   false, false);
    addKnob (pid::outGain,  "OUT",     kAmber,  true,  false);
    addKnob (pid::feedback, "FEEDBK",  kPink,   false, false);
    addKnob (pid::ring,     "RING",    kLime,   false, false);
    addKnob (pid::chaos,    "CHAOS",   kViolet, false, false);

    // --- motion -----------------------------------------------------------
    addKnob (pid::lfoSpeed, "SPEED",   kCyan,   false, false);
    addKnob (pid::lfoWave,  "SHAPE",   kLime,   false, false);
    addKnob (pid::lfoPhase, "LFO PH",  kViolet, false, false);
    addKnob (pid::spread,   "SPREAD",  kCyan,   false, false);
    addKnob (pid::msRotate, "M/S ROT", kPink,   true,  false);
    addKnob (pid::topology, "TOPOLOGY", kAmber, false, false);
    addKnob (pid::fbTone,   "FB TONE", kLime,   false, false);

    // --- freeze -----------------------------------------------------------
    freezeButton.setClickingTogglesState (true);
    freezeButton.setColour (juce::TextButton::textColourOffId, kDim);
    freezeButton.setColour (juce::TextButton::textColourOnId,  kCyan);
    addAndMakeVisible (freezeButton);
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, pid::freeze, freezeButton);

    // --- presets ---------------------------------------------------------
    for (int i = 0; i < (int) (sizeof (kPresets) / sizeof (Preset)); ++i)
        presetBox.addItem (kPresets[i].name, i + 1);

    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0)
            applyPreset (idx);
    };
    addAndMakeVisible (presetBox);

    setSize (1140, 690);
    setResizable (false, false);
}

PhaseDriftEditor::~PhaseDriftEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
LabelledKnob& PhaseDriftEditor::addKnob (const char* paramId, const char* name,
                                         juce::Colour accent, bool bipolar, bool compact)
{
    auto knob = std::make_unique<LabelledKnob> (name, accent, bipolar);
    knob->setCompact (compact);
    addAndMakeVisible (*knob);

    if (auto* param = processor.apvts.getParameter (paramId))
    {
        auto& slider = knob->getSlider();

        slider.textFromValueFunction = [param] (double v)
        {
            return param->getText ((float) v, 16);
        };

        const double defaultValue = param->convertFrom0to1 (param->getDefaultValue());
        slider.setDoubleClickReturnValue (true, defaultValue);
        sliderAttachments.push_back (
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                processor.apvts, paramId, slider));
    }

    knobs.push_back (std::move (knob));
    return *knobs.back();
}

void PhaseDriftEditor::applyPreset (int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= (int) (sizeof (kPresets) / sizeof (Preset)))
        return;

    const Preset& preset = kPresets[presetIndex];

    for (int i = 0; i < preset.count; ++i)
    {
        if (auto* param = processor.apvts.getParameter (preset.values[i].id))
        {
            param->setValueNotifyingHost (
                param->convertTo0to1 (preset.values[i].value));
        }
    }
}

//==============================================================================
void PhaseDriftEditor::resized()
{
    const int margin = 24;
    const int cardY  = 104;
    const int cardH  = 252;
    const int gap    = 14;
    const int cardW  = (getWidth() - margin * 2 - gap * (pid::numBands - 1)) / pid::numBands;

    for (int b = 0; b < pid::numBands; ++b)
        bandRects[b] = { margin + b * (cardW + gap), cardY, cardW, cardH };

    // band knobs (16) -------------------------------------------------------
    for (int b = 0; b < pid::numBands; ++b)
    {
        const auto r = bandRects[b];
        const int cellW = r.getWidth() / 2;
        const int cellH = (r.getHeight() - 46) / 2;

        for (int slot = 0; slot < 4; ++slot)
        {
            const int col = slot % 2;
            const int row = slot / 2;
            const int x = r.getX() + col * cellW + (cellW - 120) / 2;
            const int y = r.getY() + 44 + row * cellH;
            knobs[(size_t) (b * 4 + slot)]->setBounds (x, y, 120, cellH);
        }
    }

    // core row (8) ----------------------------------------------------------
    const int coreY   = 384;
    const int coreH   = 112;
    const int cellW   = (getWidth() - margin * 2) / 8;

    for (int i = 0; i < 8; ++i)
        knobs[(size_t) (16 + i)]->setBounds (margin + i * cellW + (cellW - 128) / 2, coreY, 128, coreH);

    // motion row (7 + freeze) -----------------------------------------------
    const int motionY = 522;
    const int motionH = 112;

    for (int i = 0; i < 7; ++i)
        knobs[(size_t) (24 + i)]->setBounds (margin + i * cellW + (cellW - 128) / 2, motionY, 128, motionH);

    freezeButton.setBounds (margin + 7 * cellW + (cellW - 104) / 2, motionY + 34, 104, 44);

    presetBox.setBounds (getWidth() - margin - 210, 34, 210, 30);
}

//==============================================================================
void PhaseDriftEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // background -----------------------------------------------------------
    {
        juce::ColourGradient bg (kBg, 0.0f, 0.0f, kBgDeep, 0.0f, (float) getHeight(), false);
        g.setGradientFill (bg);
        g.fillRect (bounds);
    }

    // top glow -------------------------------------------------------------
    {
        juce::ColourGradient glow (kCyan.withAlpha (0.06f), bounds.getCentreX(), -40.0f,
                                   kCyan.withAlpha (0.0f),      bounds.getCentreX(), 260.0f,
                                   true);
        g.setGradientFill (glow);
        g.fillRect (bounds.removeFromTop (300.0f).toNearestInt());
    }

    // faint vertical grid --------------------------------------------------
    g.setColour (juce::Colours::white.withAlpha (0.02f));
    for (int x = 0; x < getWidth(); x += 57)
        g.drawVerticalLine (x, 100.0f, (float) getHeight() - 20.0f);

    // header ---------------------------------------------------------------
    {
        auto logo = juce::Rectangle<float> (26.0f, 24.0f, 44.0f, 44.0f);
        juce::ColourGradient lg (kCyan.withAlpha (0.85f), logo.getTopLeft(),
                                 kViolet.withAlpha (0.75f), logo.getBottomRight(), false);
        g.setGradientFill (lg);
        g.fillRoundedRectangle (logo, 11.0f);

        g.setColour (juce::Colour (0xff05070a));
        for (int k = 0; k < 3; ++k)
        {
            juce::Path p;
            for (int i = 0; i <= 24; ++i)
            {
                const float t = (float) i / 24.0f;
                const float xx = logo.getX() + 9.0f + t * (logo.getWidth() - 18.0f);
                const float yy = logo.getCentreY() + std::sin (t * juce::MathConstants<float>::twoPi
                                                               + (float) k * 0.75f) * 7.0f;
                if (i == 0) p.startNewSubPath (xx, yy);
                else        p.lineTo (xx, yy);
            }
            g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }

        drawTracked (g, "PHASE DRIFT", juce::Font (29.0f, juce::Font::bold), kText,
                     { 84.0f, 16.0f, 460.0f, 34.0f }, juce::Justification::centredLeft, 2.4f);

        drawTracked (g, "EXPERIMENTAL PHASE-DELAY PROCESSOR", juce::Font (8.6f), kDim,
                     { 86.0f, 52.0f, 460.0f, 16.0f }, juce::Justification::centredLeft, 2.2f);

        drawTracked (g, "PRESET", juce::Font (8.0f), kDim,
                     { (float) (getWidth() - 24 - 210), 20.0f, 210.0f, 12.0f },
                     juce::Justification::centredLeft, 1.8f);

        g.setColour (kBorder.withAlpha (0.9f));
        g.drawHorizontalLine (96, 24.0f, (float) getWidth() - 24.0f);
    }

    // section titles -------------------------------------------------------
    drawTracked (g, "SPECTRAL BANDS", juce::Font (8.4f), kDim,
                 { 24.0f, 86.0f, 300.0f, 12.0f }, juce::Justification::centredLeft, 2.0f);
    drawTracked (g, "CORE", juce::Font (8.4f), kDim,
                 { 24.0f, 366.0f, 300.0f, 12.0f }, juce::Justification::centredLeft, 2.0f);
    drawTracked (g, "MOTION", juce::Font (8.4f), kDim,
                 { 24.0f, 504.0f, 300.0f, 12.0f }, juce::Justification::centredLeft, 2.0f);

    // band cards -----------------------------------------------------------
    for (int b = 0; b < pid::numBands; ++b)
    {
        const auto r = bandRects[b].toFloat();
        const juce::Colour accent = kBandAccents[b];

        g.setColour (kPanel.withAlpha (0.92f));
        g.fillRoundedRectangle (r, 9.0f);

        g.setColour (kBorder);
        g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.0f);

        g.setColour (accent.withAlpha (0.55f));
        g.fillRoundedRectangle (r.getX() + 14.0f, r.getY() + 0.5f,
                                r.getWidth() - 28.0f, 1.6f, 0.8f);

        // header of the card
        drawTracked (g, "BAND " + juce::String (b + 1), juce::Font (10.0f, juce::Font::bold),
                     accent, { r.getX() + 16.0f, r.getY() + 12.0f, 140.0f, 16.0f },
                     juce::Justification::centredLeft, 1.4f);

        drawTracked (g, kBandNames[b], juce::Font (8.2f), kDim,
                     { r.getRight() - 116.0f, r.getY() + 13.0f, 100.0f, 14.0f },
                     juce::Justification::right, 1.2f);
    }

    // footer ---------------------------------------------------------------
    g.setColour (kBorder.withAlpha (0.8f));
    g.drawHorizontalLine (getHeight() - 46, 24.0f, (float) getWidth() - 24.0f);

    drawTracked (g, "PHASE DRIFT  v0.9.0", juce::Font (8.2f), kDim,
                 { 24.0f, (float) getHeight() - 34.0f, 340.0f, 12.0f },
                 juce::Justification::centredLeft, 1.4f);

    drawTracked (g, "DOUBLE-CLICK A KNOB TO RESET  /  DRAG VERTICALLY TO FINE-TUNE",
                 juce::Font (8.2f), kDim.withAlpha (0.8f),
                 { 360.0f, (float) getHeight() - 34.0f, 520.0f, 12.0f },
                 juce::Justification::centredLeft, 1.2f);

    drawTracked (g, "AGPL-3.0", juce::Font (8.2f), kDim,
                 { (float) getWidth() - 24.0f - 160.0f, (float) getHeight() - 34.0f, 160.0f, 12.0f },
                 juce::Justification::right, 1.4f);
}
