#include "PluginEditor.h"

//==============================================================================
namespace
{
    const juce::Colour kBg      { 0xff080a0d };
    const juce::Colour kBgDeep  { 0xff04060a };
    const juce::Colour kPanel   { 0xff0c1015 };
    const juce::Colour kBorder  { 0xff1a2430 };
    const juce::Colour kDim     { 0xff7d8e9d };
    const juce::Colour kText    { 0xffe2edf3 };
    const juce::Colour kCyan    { 0xff7ecbd6 };
    const juce::Colour kPink    { 0xffcf8fa6 };
    const juce::Colour kLime    { 0xffb6c98c };
    const juce::Colour kViolet  { 0xffa79ac6 };
    const juce::Colour kAmber   { 0xffd8b58c };

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
        { pid::feedback, 0.20f }, { pid::fbTone, 0.55f }, { pid::chaos, 0.15f },
        { pid::topology, 0.2f }, { pid::spread, 0.35f }, { pid::msRotate, 0.0f },
        { pid::lfoSpeed, 1.0f }, { pid::lfoWave, 0.3f }, { pid::lfoPhase, 0.0f },
        { pid::bandFreq (0), 90.0f },  { pid::bandPhase (0), 0.35f },
        { pid::bandDepth (0), 0.22f }, { pid::bandRate (0), 0.17f },
        { pid::bandEcho (0), 0.5f },
        { pid::bandFreq (1), 720.0f }, { pid::bandPhase (1), -0.2f },
        { pid::bandDepth (1), 0.18f }, { pid::bandRate (1), 0.43f },
        { pid::bandEcho (1), 0.75f },
        { pid::bandFreq (2), 3100.0f }, { pid::bandPhase (2), 0.5f },
        { pid::bandDepth (2), 0.26f }, { pid::bandRate (2), 0.71f },
        { pid::bandEcho (2), 1.0f },
        { pid::bandFreq (3), 9500.0f }, { pid::bandPhase (3), 0.15f },
        { pid::bandDepth (3), 0.12f }, { pid::bandRate (3), 1.13f },
        { pid::bandEcho (3), 1.5f },
        { pid::echoTime (0), 240.0f }, { pid::echoTime (1), 380.0f },
        { pid::echoFb (0), 0.42f }, { pid::echoFb (1), 0.50f },
        { pid::echoDamp (0), 0.45f }, { pid::echoDamp (1), 0.55f },
        { pid::echoMix (0), 0.55f }, { pid::echoMix (1), 0.60f }
    };

    const PresetValue glassValues[] {
        { pid::inGain, -3.0f }, { pid::mix, 1.0f }, { pid::outGain, 1.0f },
        { pid::drive, 0.22f }, { pid::ring, 0.05f }, { pid::tilt, 0.15f },
        { pid::feedback, 0.30f }, { pid::fbTone, 0.8f }, { pid::chaos, 0.1f },
        { pid::topology, 0.62f }, { pid::spread, 0.62f }, { pid::msRotate, 12.0f },
        { pid::lfoSpeed, 0.75f }, { pid::lfoWave, 0.34f }, { pid::lfoPhase, 40.0f },
        { pid::bandFreq (0), 150.0f },  { pid::bandPhase (0), 0.62f },
        { pid::bandDepth (0), 0.38f },  { pid::bandRate (0), 0.11f },
        { pid::bandEcho (0), 0.25f },
        { pid::bandFreq (1), 1250.0f }, { pid::bandPhase (1), -0.42f },
        { pid::bandDepth (1), 0.32f },  { pid::bandRate (1), 0.27f },
        { pid::bandEcho (1), 0.5f },
        { pid::bandFreq (2), 5400.0f }, { pid::bandPhase (2), 0.78f },
        { pid::bandDepth (2), 0.48f },  { pid::bandRate (2), 0.55f },
        { pid::bandEcho (2), 0.85f },
        { pid::bandFreq (3), 15000.0f }, { pid::bandPhase (3), -0.24f },
        { pid::bandDepth (3), 0.22f },   { pid::bandRate (3), 0.92f },
        { pid::bandEcho (3), 1.25f },
        { pid::echoTime (0), 180.0f }, { pid::echoTime (1), 265.0f },
        { pid::echoFb (0), 0.35f }, { pid::echoFb (1), 0.40f },
        { pid::echoDamp (0), 0.25f }, { pid::echoDamp (1), 0.30f },
        { pid::echoMix (0), 0.62f }, { pid::echoMix (1), 0.70f }
    };

    const PresetValue ghostValues[] {
        { pid::inGain, 0.0f }, { pid::mix, 1.0f }, { pid::outGain, -1.5f },
        { pid::drive, 0.06f }, { pid::ring, 0.0f }, { pid::tilt, -0.35f },
        { pid::feedback, 0.55f }, { pid::fbTone, 0.75f }, { pid::chaos, 0.28f },
        { pid::topology, 1.0f }, { pid::spread, 0.22f }, { pid::msRotate, 0.0f },
        { pid::lfoSpeed, 0.3f }, { pid::lfoWave, 0.5f }, { pid::lfoPhase, 0.0f },
        { pid::bandFreq (0), 62.0f },  { pid::bandPhase (0), 0.86f },
        { pid::bandDepth (0), 0.5f },  { pid::bandRate (0), 0.06f },
        { pid::bandEcho (0), 0.5f },
        { pid::bandFreq (1), 480.0f }, { pid::bandPhase (1), -0.6f },
        { pid::bandDepth (1), 0.42f }, { pid::bandRate (1), 0.13f },
        { pid::bandEcho (1), 1.0f },
        { pid::bandFreq (2), 2400.0f }, { pid::bandPhase (2), 0.9f },
        { pid::bandDepth (2), 0.6f },   { pid::bandRate (2), 0.21f },
        { pid::bandEcho (2), 1.5f },
        { pid::bandFreq (3), 8200.0f }, { pid::bandPhase (3), 0.52f },
        { pid::bandDepth (3), 0.36f },  { pid::bandRate (3), 0.33f },
        { pid::bandEcho (3), 2.0f },
        { pid::echoTime (0), 420.0f }, { pid::echoTime (1), 470.0f },
        { pid::echoFb (0), 0.62f }, { pid::echoFb (1), 0.66f },
        { pid::echoDamp (0), 0.70f }, { pid::echoDamp (1), 0.75f },
        { pid::echoMix (0), 0.55f }, { pid::echoMix (1), 0.60f }
    };

    const PresetValue dopplerValues[] {
        { pid::inGain, 0.0f }, { pid::mix, 0.88f }, { pid::outGain, 0.0f },
        { pid::drive, 0.25f }, { pid::ring, 0.35f }, { pid::tilt, 0.0f },
        { pid::feedback, 0.25f }, { pid::fbTone, 0.4f }, { pid::chaos, 0.12f },
        { pid::topology, 0.3f }, { pid::spread, 0.85f }, { pid::msRotate, 45.0f },
        { pid::lfoSpeed, 2.2f }, { pid::lfoWave, 0.15f }, { pid::lfoPhase, 90.0f },
        { pid::bandFreq (0), 42.0f },  { pid::bandPhase (0), -0.92f },
        { pid::bandDepth (0), 0.72f }, { pid::bandRate (0), 0.05f },
        { pid::bandEcho (0), 1.0f },
        { pid::bandFreq (1), 320.0f }, { pid::bandPhase (1), 0.82f },
        { pid::bandDepth (1), 0.62f }, { pid::bandRate (1), 0.09f },
        { pid::bandEcho (1), 1.12f },
        { pid::bandFreq (2), 1900.0f }, { pid::bandPhase (2), -0.72f },
        { pid::bandDepth (2), 0.55f },  { pid::bandRate (2), 0.14f },
        { pid::bandEcho (2), 1.28f },
        { pid::bandFreq (3), 6200.0f }, { pid::bandPhase (3), 0.9f },
        { pid::bandDepth (3), 0.42f },  { pid::bandRate (3), 0.22f },
        { pid::bandEcho (3), 1.45f },
        { pid::echoTime (0), 155.0f }, { pid::echoTime (1), 215.0f },
        { pid::echoFb (0), 0.30f }, { pid::echoFb (1), 0.36f },
        { pid::echoDamp (0), 0.20f }, { pid::echoDamp (1), 0.25f },
        { pid::echoMix (0), 0.48f }, { pid::echoMix (1), 0.55f }
    };

    const PresetValue meltValues[] {
        { pid::inGain, -2.0f }, { pid::mix, 1.0f }, { pid::outGain, -1.0f },
        { pid::drive, 0.55f }, { pid::ring, 0.08f }, { pid::tilt, 0.2f },
        { pid::feedback, 0.45f }, { pid::fbTone, 0.3f }, { pid::chaos, 0.65f },
        { pid::topology, 0.45f }, { pid::spread, 0.45f }, { pid::msRotate, -25.0f },
        { pid::lfoSpeed, 0.55f }, { pid::lfoWave, 0.42f }, { pid::lfoPhase, 200.0f },
        { pid::bandFreq (0), 120.0f }, { pid::bandPhase (0), 0.42f },
        { pid::bandDepth (0), 0.36f }, { pid::bandRate (0), 0.35f },
        { pid::bandEcho (0), 0.75f },
        { pid::bandFreq (1), 900.0f }, { pid::bandPhase (1), -0.52f },
        { pid::bandDepth (1), 0.46f }, { pid::bandRate (1), 0.61f },
        { pid::bandEcho (1), 1.0f },
        { pid::bandFreq (2), 4100.0f }, { pid::bandPhase (2), 0.72f },
        { pid::bandDepth (2), 0.5f },   { pid::bandRate (2), 1.1f },
        { pid::bandEcho (2), 1.75f },
        { pid::bandFreq (3), 12000.0f }, { pid::bandPhase (3), 0.3f },
        { pid::bandDepth (3), 0.3f },    { pid::bandRate (3), 1.7f },
        { pid::bandEcho (3), 2.25f },
        { pid::echoTime (0), 305.0f }, { pid::echoTime (1), 330.0f },
        { pid::echoFb (0), 0.58f }, { pid::echoFb (1), 0.52f },
        { pid::echoDamp (0), 0.55f }, { pid::echoDamp (1), 0.45f },
        { pid::echoMix (0), 0.62f }, { pid::echoMix (1), 0.66f }
    };

    const Preset kPresets[] {
        { "INIT",          initValues,    (int) (sizeof (initValues)    / sizeof (PresetValue)) },
        { "GLASS ROTOR",   glassValues,   (int) (sizeof (glassValues)   / sizeof (PresetValue)) },
        { "GHOST COMB",    ghostValues,   (int) (sizeof (ghostValues)   / sizeof (PresetValue)) },
        { "DOPPLER WELL",  dopplerValues, (int) (sizeof (dopplerValues) / sizeof (PresetValue)) },
        { "TAPE MELT",     meltValues,    (int) (sizeof (meltValues)    / sizeof (PresetValue)) }
    };

    // Knob indices inside the editor's knob vector -------------------------
    enum KnobGroup
    {
        kBandKnobs     = 0,     // 16: 4 bands x 4
        kBandEchoKnobs = 16,    // 4:  per-band delay ratio
        kCoreKnobs     = 20,    // 8
        kMotionKnobs   = 28,    // 7
        kEchoLKnobs    = 35,    // 4
        kEchoRKnobs    = 39,    // 4
        kNumKnobs      = 43
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

    // --- per-band echo ratio ---------------------------------------------
    for (int b = 0; b < pid::numBands; ++b)
        addKnob (pid::bandEcho (b), "ECHO / E", kBandAccents[b], false, true);

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
    addKnob (pid::lfoSpeed, "SPEED",    kCyan,   false, false);
    addKnob (pid::lfoWave,  "SHAPE",    kLime,   false, false);
    addKnob (pid::lfoPhase, "LFO PH",   kViolet, false, false);
    addKnob (pid::spread,   "SPREAD",   kCyan,   false, false);
    addKnob (pid::msRotate, "M/S ROT",  kPink,   true,  false);
    addKnob (pid::topology, "TOPOLOGY", kAmber,  false, false);
    addKnob (pid::fbTone,   "FB TONE",  kLime,   false, false);

    // --- echo, left then right -------------------------------------------
    addKnob (pid::echoTime (0), "TIME",   kCyan,  false, false);
    addKnob (pid::echoFb (0),   "FEEDBK", kPink,  false, false);
    addKnob (pid::echoDamp (0), "DAMP",   kLime,  false, false);
    addKnob (pid::echoMix (0),  "LEVEL",  kAmber, false, false);

    addKnob (pid::echoTime (1), "TIME",   kCyan,  false, false);
    addKnob (pid::echoFb (1),   "FEEDBK", kPink,  false, false);
    addKnob (pid::echoDamp (1), "DAMP",   kLime,  false, false);
    addKnob (pid::echoMix (1),  "LEVEL",  kAmber, false, false);

    jassert ((int) knobs.size() == kNumKnobs);

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

    setSize (1360, 860);
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
    const int gap    = 16;

    // ---- spectral bands ---------------------------------------------------
    const int cardY = 74;
    const int cardH = 190;
    const int cardW = (getWidth() - margin * 2 - gap * 3) / 4;

    for (int b = 0; b < pid::numBands; ++b)
        bandRects[b] = { margin + b * (cardW + gap), cardY, cardW, cardH };

    for (int b = 0; b < pid::numBands; ++b)
    {
        const auto r = bandRects[b];
        const int cellW = r.getWidth() / 2;
        const int cellH = (r.getHeight() - 26) / 2;
        const int knobW = juce::jmin (104, cellW - 12);

        for (int slot = 0; slot < 4; ++slot)
        {
            const int col = slot % 2;
            const int row = slot / 2;
            const int x = r.getX() + col * cellW + (cellW - knobW) / 2;
            const int y = r.getY() + 26 + row * cellH;
            knobs[(size_t) (kBandKnobs + b * 4 + slot)]->setBounds (x, y, knobW, cellH);
        }
    }

    // ---- per-band echo strip ---------------------------------------------
    bandEchoRect = { margin, cardY + cardH + 8, getWidth() - margin * 2, 106 };

    for (int b = 0; b < pid::numBands; ++b)
        knobs[(size_t) (kBandEchoKnobs + b)]->setBounds (bandRects[b].getX(),
                                                        bandEchoRect.getY() + 26,
                                                        cardW, 78);

    // ---- core row ---------------------------------------------------------
    const int coreY = bandEchoRect.getBottom() + 20;
    const int cellW = (getWidth() - margin * 2) / 8;

    coreRect = { margin, coreY, getWidth() - margin * 2, 110 };

    for (int i = 0; i < 8; ++i)
        knobs[(size_t) (kCoreKnobs + i)]->setBounds (margin + i * cellW + (cellW - 130) / 2,
                                                     coreY, 130, 110);

    // ---- motion row -------------------------------------------------------
    const int motionY = coreY + 132;

    motionRect = { margin, motionY, getWidth() - margin * 2, 110 };

    for (int i = 0; i < 7; ++i)
        knobs[(size_t) (kMotionKnobs + i)]->setBounds (margin + i * cellW + (cellW - 130) / 2,
                                                       motionY, 130, 110);

    freezeButton.setBounds (margin + 7 * cellW + (cellW - 104) / 2, motionY + 33, 104, 44);

    // ---- echo panels ------------------------------------------------------
    const int echoY = motionY + 110 + 10;
    const int panelW = (getWidth() - margin * 2 - gap) / 2;

    echoLeftRect  = { margin, echoY, panelW, 170 };
    echoRightRect = { margin + panelW + gap, echoY, panelW, 170 };

    for (int c = 0; c < 2; ++c)
    {
        const auto panel = (c == 0 ? echoLeftRect : echoRightRect);
        const int knobW = panel.getWidth() / 4;

        for (int i = 0; i < 4; ++i)
        {
            const int idx = (c == 0 ? kEchoLKnobs : kEchoRKnobs) + i;
            knobs[(size_t) idx]->setBounds (panel.getX() + i * knobW,
                                            panel.getY() + 34, knobW, 130);
        }
    }

    presetBox.setBounds (getWidth() - margin - 200, 16, 200, 28);
}

//==============================================================================
void PhaseDriftEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // flat background -------------------------------------------------------
    g.setColour (kBg);
    g.fillRect (bounds);

    {
        juce::ColourGradient bg (kBgDeep, 0.0f, 0.0f, kBg, 0.0f, (float) getHeight(), false);
        g.setGradientFill (bg);
        g.fillRect (bounds.removeFromTop ((float) getHeight()));
    }

    // header ------------------------------------------------------------------
    drawTracked (g, "PHASE DRIFT", juce::Font (26.0f, juce::Font::bold), kText,
                 { 24.0f, 14.0f, 620.0f, 30.0f }, juce::Justification::centredLeft, 2.2f);

    drawTracked (g, "PER-BAND PHASE + INDEPENDENT STEREO ECHO", juce::Font (10.0f), kDim,
                 { 25.0f, 46.0f, 620.0f, 14.0f }, juce::Justification::centredLeft, 2.2f);

    drawTracked (g, "PRESET", juce::Font (9.6f), kDim,
                 { (float) (getWidth() - 24 - 200) - 62.0f, 20.0f, 56.0f, 20.0f },
                 juce::Justification::centredRight, 1.8f);

    g.setColour (kBorder);
    g.drawHorizontalLine (66, 24.0f, (float) getWidth() - 24.0f);

    // section titles -------------------------------------------------------
    drawTracked (g, "SPECTRAL BANDS", juce::Font (10.0f), kDim,
                 { 24.0f, 62.0f, 300.0f, 12.0f }, juce::Justification::centredLeft, 2.0f);

    drawTracked (g, "BAND ECHO  /  DELAY RATIO", juce::Font (10.0f), kDim,
                 { 36.0f, (float) bandEchoRect.getY() + 9, 460.0f, 14.0f },
                 juce::Justification::centredLeft, 2.0f);

    drawTracked (g, "CORE", juce::Font (10.0f), kDim,
                 { 24.0f, (float) coreRect.getY() - 14, 300.0f, 12.0f },
                 juce::Justification::centredLeft, 2.0f);

    drawTracked (g, "MOTION", juce::Font (10.0f), kDim,
                 { 24.0f, (float) motionRect.getY() - 14, 300.0f, 12.0f },
                 juce::Justification::centredLeft, 2.0f);

    // band cards -----------------------------------------------------------
    for (int b = 0; b < pid::numBands; ++b)
    {
        const auto r = bandRects[b].toFloat();
        const juce::Colour accent = kBandAccents[b];

        g.setColour (kPanel);
        g.fillRoundedRectangle (r, 6.0f);

        g.setColour (kBorder);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

        g.setColour (accent.withAlpha (0.55f));
        g.fillRect (r.getX() + 14.0f, r.getY() + 0.5f, r.getWidth() - 28.0f, 1.6f);

        drawTracked (g, "BAND " + juce::String (b + 1), juce::Font (12.0f, juce::Font::bold),
                     accent, { r.getX() + 16.0f, r.getY() + 10.0f, 140.0f, 16.0f },
                     juce::Justification::centredLeft, 1.4f);

        drawTracked (g, kBandNames[b], juce::Font (9.8f), kDim,
                     { r.getRight() - 130.0f, r.getY() + 9.0f, 114.0f, 16.0f },
                     juce::Justification::right, 1.2f);
    }

    // band echo strip -------------------------------------------------------
    {
        const auto r = bandEchoRect.toFloat();

        g.setColour (kPanel.withAlpha (0.85f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (kBorder);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);

        for (int b = 0; b < pid::numBands; ++b)
        {
            const juce::Colour accent = kBandAccents[b];
            g.setColour (accent.withAlpha (0.25f));
            g.fillRect ((float) bandRects[b].getX() + 6.0f, r.getY() + 26.0f,
                        2.0f, r.getHeight() - 40.0f);
        }
    }

    // echo panels -----------------------------------------------------------
    for (int c = 0; c < 2; ++c)
    {
        const auto r = (c == 0 ? echoLeftRect : echoRightRect).toFloat();
        const juce::Colour accent = c == 0 ? kCyan : kPink;

        g.setColour (kPanel);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (kBorder);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);

        g.setColour (accent.withAlpha (0.55f));
        g.fillRect (r.getX() + 14.0f, r.getY() + 0.5f, r.getWidth() - 28.0f, 1.6f);

        drawTracked (g, c == 0 ? "ECHO  /  LEFT" : "ECHO  /  RIGHT",
                     juce::Font (12.0f, juce::Font::bold), accent,
                     { r.getX() + 16.0f, r.getY() + 10.0f, 220.0f, 16.0f },
                     juce::Justification::centredLeft, 1.6f);

        g.setColour (kBorder.withAlpha (0.9f));
        g.fillRect (r.getX() + 16.0f, r.getY() + 30.0f, r.getWidth() - 32.0f, 1.0f);
    }

    // footer ---------------------------------------------------------------
    g.setColour (kBorder.withAlpha (0.8f));
    g.drawHorizontalLine (getHeight() - 30, 24.0f, (float) getWidth() - 24.0f);

    drawTracked (g, "v0.9.0", juce::Font (9.8f), kDim,
                 { 24.0f, (float) getHeight() - 22.0f, 120.0f, 12.0f },
                 juce::Justification::centredLeft, 1.4f);

    drawTracked (g, "DOUBLE-CLICK A KNOB TO RESET   /   ALT-DRAG FOR FINE STEPS",
                 juce::Font (9.8f), kDim.withAlpha (0.85f),
                 { 360.0f, (float) getHeight() - 22.0f, 620.0f, 12.0f },
                 juce::Justification::centredRight, 1.2f);

    drawTracked (g, "AGPL-3.0", juce::Font (9.8f), kDim,
                 { (float) getWidth() - 24.0f - 160.0f, (float) getHeight() - 22.0f, 160.0f, 12.0f },
                 juce::Justification::right, 1.4f);
}




