#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
namespace
{
    /** Logarithmic range - musical knob feel. */
    juce::NormalisableRange<float> logRange (float lo, float hi)
    {
        return juce::NormalisableRange<float> (
            lo, hi,
            [] (float a, float b, float t) { return a * std::pow (b / a, t); },
            [] (float a, float b, float v) { return std::log (v / a) / std::log (b / a); });
    }

    juce::String hzText (float v)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                            : juce::String (juce::roundToInt (v)) + " Hz";
    }

    juce::String pctText (float v)   { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
    juce::String dbText  (float v)   { return (v >= 0 ? "+" : "") + juce::String (v, 1) + " dB"; }

    juce::String degText (float v)
    {
        // NB: never build the degree sign from a narrow string literal - MSVC on a
        // CP936 machine encodes "\u00b0" as GBK bytes (0xA1 0xE3), which then decode
        // into broken code points and can wedge JUCE's UTF-8 scanner (hard crash in
        // Font::getStringWidthFloat). Build it straight from the code point instead.
        return juce::String (juce::roundToInt (v))
               + juce::String::charToString ((juce::juce_wchar) 0x00b0);
    }

    juce::String xText   (float v)   { return juce::String (v, 2) + "x"; }

    juce::String msText  (float v)
    {
        if (v >= 1000.0f)
            return juce::String (v / 1000.0f, 2) + " s";

        if (v < 10.0f)
            return juce::String (v, 1) + " ms";

        return juce::String (juce::roundToInt (v)) + " ms";
    }

    /** Skewed sine -> triangle -> saw LFO. */
    inline float lfoValue (float phase, float shape) noexcept
    {
        const float sp = phase - std::floor (phase);
        const float sine = std::sin (juce::MathConstants<float>::twoPi * sp);
        const float tri  = 1.0f - 4.0f * std::abs (sp - 0.5f);
        const float saw  = 2.0f * sp - 1.0f;

        if (shape < 0.5f)
            return sine + (tri - sine) * (shape * 2.0f);

        return tri + (saw - tri) * ((shape - 0.5f) * 2.0f);
    }
}

//==============================================================================
void PhaseDriftProcessor::Biquad::setLowpass (float fc, float sampleRate) noexcept
{
    fc = juce::jlimit (15.0f, sampleRate * 0.45f, fc);
    const float w0 = juce::MathConstants<float>::twoPi * fc / sampleRate;
    const float cw = std::cos (w0), sw = std::sin (w0);
    const float alpha = sw * 0.7071068f;
    const float a0 = 1.0f + alpha;

    b0 = ((1.0f - cw) * 0.5f) / a0;
    b1 = (1.0f - cw) / a0;
    b2 = b0;
    a1 = (-2.0f * cw) / a0;
    a2 = (1.0f - alpha) / a0;
}

void PhaseDriftProcessor::Biquad::setHighpass (float fc, float sampleRate) noexcept
{
    fc = juce::jlimit (15.0f, sampleRate * 0.45f, fc);
    const float w0 = juce::MathConstants<float>::twoPi * fc / sampleRate;
    const float cw = std::cos (w0), sw = std::sin (w0);
    const float alpha = sw * 0.7071068f;
    const float a0 = 1.0f + alpha;

    b0 = ((1.0f + cw) * 0.5f) / a0;
    b1 = -(1.0f + cw) / a0;
    b2 = b0;
    a1 = (-2.0f * cw) / a0;
    a2 = (1.0f - alpha) / a0;
}

//==============================================================================
void PhaseDriftProcessor::EchoTap::prepare (int maxSamples)
{
    buf.assign ((size_t) juce::jmax (8, maxSamples), 0.0f);
    reset();
}

void PhaseDriftProcessor::EchoTap::reset() noexcept
{
    std::fill (buf.begin(), buf.end(), 0.0f);
    writePos = 0;
    delaySamples = 0.0f;
    dampState = 0.0f;
}

void PhaseDriftProcessor::Channel::reset() noexcept
{
    for (int b = 0; b < pid::numBands; ++b)
    {
        for (int s = 0; s < 4; ++s)
        {
            bandAP[b][s].reset();
            seriesAP[b][s].reset();
        }
        lfoPhase[b] = 0.0f;
    }

    for (int s = 0; s < 3; ++s)
    {
        splitLP[s].reset();
        splitHP[s].reset();
    }

    for (auto& e : echo)
        e.reset();

    tiltLP.reset();
    postLP.reset();
    fbLP.reset();
    fbMemory = 0.0f;
}

//==============================================================================
PhaseDriftProcessor::PhaseDriftProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PhaseDriftState", createParameterLayout())
{
}

PhaseDriftProcessor::~PhaseDriftProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PhaseDriftProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&layout] (const char* id, const char* name,
                               juce::NormalisableRange<float> range, float def,
                               juce::String (*textFn) (float),
                               const char* group)
    {
        juce::ignoreUnused (group);

        auto attr = juce::AudioParameterFloatAttributes()
                        .withStringFromValueFunction ([textFn] (float v, int) { return textFn (v); });

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def, attr));
    };

    // --- global -----------------------------------------------------------
    addFloat (pid::inGain,   "Input Gain",  { -24.0f, 24.0f, 0.1f },  0.0f,   dbText,  "Core");
    addFloat (pid::mix,      "Dry / Wet",   { 0.0f, 1.0f, 0.001f },   1.0f,   pctText, "Core");
    addFloat (pid::outGain,  "Output Gain", { -24.0f, 24.0f, 0.1f },  0.0f,   dbText,  "Core");
    addFloat (pid::drive,    "Wavefold",    { 0.0f, 1.0f, 0.001f },   0.12f,  pctText, "Core");
    addFloat (pid::ring,     "Ring Mod",    { 0.0f, 1.0f, 0.001f },   0.0f,   pctText, "Core");
    addFloat (pid::tilt,     "Pre Tilt",    { -1.0f, 1.0f, 0.001f },  0.0f,   pctText, "Core");

    addFloat (pid::feedback, "Feedback",    { 0.0f, 0.95f, 0.001f },  0.20f,  pctText, "Feedback");
    addFloat (pid::fbTone,   "Feedback Tone", { 0.0f, 1.0f, 0.001f }, 0.55f,  pctText, "Feedback");
    addFloat (pid::chaos,    "Chaos",       { 0.0f, 1.0f, 0.001f },   0.15f,  pctText, "Feedback");
    addFloat (pid::topology, "Series / Parallel", { 0.0f, 1.0f, 0.001f }, 0.20f, pctText, "Feedback");

    addFloat (pid::spread,   "Stereo Spread", { 0.0f, 1.0f, 0.001f }, 0.35f,  pctText, "Motion");
    addFloat (pid::msRotate, "M / S Rotate", { -180.0f, 180.0f, 1.0f }, 0.0f, degText, "Motion");
    addFloat (pid::lfoSpeed, "LFO Speed",   logRange (0.05f, 10.0f),  1.0f,   xText,   "Motion");
    addFloat (pid::lfoWave,  "LFO Shape",   { 0.0f, 1.0f, 0.001f },   0.30f,  pctText, "Motion");
    addFloat (pid::lfoPhase, "LFO Phase",   { 0.0f, 360.0f, 1.0f },   0.0f,   degText, "Motion");

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { pid::freeze, 1 }, "Freeze", false));

    // --- bands --------------------------------------------------------------
    static const float defaultFreq[4]  = { 90.0f, 720.0f, 3100.0f, 9500.0f };
    static const float defaultPhase[4] = { 0.35f, -0.20f, 0.50f, 0.15f };
    static const float defaultDepth[4] = { 0.22f, 0.18f, 0.26f, 0.12f };
    static const float defaultRate[4]  = { 0.17f, 0.43f, 0.71f, 1.13f };
    static const float defaultEcho[4]  = { 0.50f, 0.75f, 1.00f, 1.50f };
    static const char* names[4]        = { "Band 1", "Band 2", "Band 3", "Band 4" };

    for (int b = 0; b < pid::numBands; ++b)
    {
        const juce::String suffix = " " + juce::String (b + 1);

        addFloat (pid::bandFreq (b),  ("Centre Freq" + suffix).toRawUTF8(),
                  logRange (20.0f, 20000.0f), defaultFreq[b],  hzText,  names[b]);
        addFloat (pid::bandPhase (b), ("Phase" + suffix).toRawUTF8(),
                  { -1.0f, 1.0f, 0.001f }, defaultPhase[b],     pctText, names[b]);
        addFloat (pid::bandDepth (b), ("Mod Depth" + suffix).toRawUTF8(),
                  { 0.0f, 1.0f, 0.001f }, defaultDepth[b],       pctText, names[b]);
        addFloat (pid::bandRate (b),  ("Mod Rate" + suffix).toRawUTF8(),
                  logRange (0.02f, 12.0f), defaultRate[b],       xText,   names[b]);
        addFloat (pid::bandEcho (b),  ("Echo Time Ratio" + suffix).toRawUTF8(),
                  logRange (0.25f, 3.0f), defaultEcho[b],        xText,   names[b]);
    }

    // --- echo, one independent set per output channel -------------------------
    static const float defaultTime[2] = { 240.0f, 380.0f };
    static const float defaultFb[2]   = { 0.42f, 0.50f };
    static const float defaultDamp[2] = { 0.45f, 0.55f };
    static const float defaultMix[2]  = { 0.55f, 0.60f };
    static const char* chName[2]      = { " Left", " Right" };

    for (int c = 0; c < pid::numChans; ++c)
    {
        const juce::String suffix = chName[c];

        addFloat (pid::echoTime (c), ("Echo Time" + suffix).toRawUTF8(),
                  logRange (2.0f, 1000.0f), defaultTime[c], msText,  "Echo");
        addFloat (pid::echoFb (c),   ("Echo Feedback" + suffix).toRawUTF8(),
                  { 0.0f, 0.90f, 0.001f }, defaultFb[c],     pctText, "Echo");
        addFloat (pid::echoDamp (c), ("Echo Damping" + suffix).toRawUTF8(),
                  { 0.0f, 1.0f, 0.001f }, defaultDamp[c],    pctText, "Echo");
        addFloat (pid::echoMix (c),  ("Echo Level" + suffix).toRawUTF8(),
                  { 0.0f, 1.0f, 0.001f }, defaultMix[c],     pctText, "Echo");
    }

    return layout;
}

//==============================================================================
void PhaseDriftProcessor::prepareToPlay (double sampleRate, int)
{
    sr = (float) sampleRate;

    const int maxSamples = (int) std::ceil (maxEchoSeconds * sr) + 8;

    channels.resize ((size_t) juce::jmax (1, getTotalNumOutputChannels()));

    for (auto& c : channels)
    {
        for (auto& e : c.echo)
            e.prepare (maxSamples);

        c.reset();
    }

    const float ramp = 0.02f;
    smInGain.reset (sampleRate, ramp);
    smOutGain.reset (sampleRate, ramp);
    smMix.reset (sampleRate, ramp);
    smFeedback.reset (sampleRate, ramp);
    smDrive.reset (sampleRate, ramp);
    smRing.reset (sampleRate, ramp);
    smTilt.reset (sampleRate, ramp);
    smTopo.reset (sampleRate, ramp);

    // echo times are smoothed in samples, slightly slower so time changes glide
    for (int c = 0; c < pid::numChans; ++c)
    {
        smEchoTime[c].reset (sampleRate, 0.05);
        smEchoFb[c].reset (sampleRate, 0.02);
        smEchoMix[c].reset (sampleRate, 0.02);
        echoDampK[c] = 0.5f;
    }

    tiltK = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 260.0f / sr);
    dcCoeff = 1.0f - juce::MathConstants<float>::twoPi * 8.0f / sr;

    for (int b = 0; b < pid::numBands; ++b)
    {
        bands[b].aSmooth = 0.0f;
        echoBandScale[b] = 1.0f;
    }

    dcState[0] = dcState[1] = dcLast[0] = dcLast[1] = 0.0f;
}

void PhaseDriftProcessor::releaseResources() {}

bool PhaseDriftProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainIn == mainOut;
}

//==============================================================================
void PhaseDriftProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (buffer.getNumChannels(), (int) channels.size());

    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numCh == 0 || numSamples == 0)
        return;

    // ---- read parameters -------------------------------------------------
    auto val = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float inGainDb = val (pid::inGain);
    const float outGainDb = val (pid::outGain);
    const float mixV      = val (pid::mix);
    const float driveV    = val (pid::drive);
    const float ringV     = val (pid::ring);
    const float tiltV     = val (pid::tilt);
    const float fbV       = val (pid::feedback);
    const float fbToneV   = val (pid::fbTone);
    const float chaosV    = val (pid::chaos);
    const float topoV     = val (pid::topology);
    const float spreadV   = val (pid::spread);
    const float msRotV    = val (pid::msRotate);
    const float lfoSpeedV = val (pid::lfoSpeed);
    const float waveV     = val (pid::lfoWave);
    const float lfoPhV    = val (pid::lfoPhase);
    const bool  freezeV   = val (pid::freeze) > 0.5f;

    for (int b = 0; b < pid::numBands; ++b)
    {
        bands[b].freq  = val (pid::bandFreq (b));
        bands[b].phase = val (pid::bandPhase (b));
        bands[b].depth = val (pid::bandDepth (b));
        bands[b].rate  = val (pid::bandRate (b));
        echoBandScale[b] = val (pid::bandEcho (b));
    }

    // echo, one set of values per channel ----------------------------------
    for (int c = 0; c < pid::numChans; ++c)
    {
        const float timeMs = val (pid::echoTime (c));
        const float damp   = val (pid::echoDamp (c));

        smEchoTime[c].setTargetValue (juce::jlimit (1.0f, maxEchoSeconds * sr,
                                                    timeMs * 0.001f * sr));
        smEchoFb[c].setTargetValue (val (pid::echoFb (c)));
        smEchoMix[c].setTargetValue (val (pid::echoMix (c)));

        const float fc = juce::jlimit (120.0f, sr * 0.45f, 200.0f + damp * damp * 13000.0f);
        echoDampK[c] = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / sr);
    }

    smInGain.setTargetValue  (juce::Decibels::decibelsToGain (inGainDb));
    smOutGain.setTargetValue (juce::Decibels::decibelsToGain (outGainDb));
    smMix.setTargetValue     (mixV);
    smFeedback.setTargetValue (freezeV ? juce::jmax (fbV, 0.985f) : fbV);
    smDrive.setTargetValue   (driveV);
    smRing.setTargetValue    (ringV);
    smTilt.setTargetValue    (tiltV);
    smTopo.setTargetValue    (topoV);

    waveShape     = waveV;
    spreadAmount  = spreadV;
    globalPhaseNorm = lfoPhV / 360.0f;
    chaosAmount   = chaosV;

    const float rotRad = msRotV * juce::MathConstants<float>::pi / 180.0f;
    rotCos = std::cos (rotRad);
    rotSin = std::sin (rotRad);

    toneK = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * (40.0f + fbToneV * fbToneV * 12000.0f) / sr);

    // ---- filter coefficients ---------------------------------------------
    const float cross1 = juce::jlimit (20.0f, sr * 0.45f, std::sqrt (bands[0].freq * bands[1].freq));
    const float cross2 = juce::jlimit (20.0f, sr * 0.45f, std::sqrt (bands[1].freq * bands[2].freq));
    const float cross3 = juce::jlimit (20.0f, sr * 0.45f, std::sqrt (bands[2].freq * bands[3].freq));

    float inc[pid::numBands] {};
    float aBaseT[pid::numBands] {};

    for (int b = 0; b < pid::numBands; ++b)
    {
        const float rate = bands[b].rate * lfoSpeedV;
        float jitter = 1.0f;

        if (chaosAmount > 0.0f)
            jitter = 1.0f + chaosAmount * 0.65f * (2.0f * (float) juce::Random::getSystemRandom().nextDouble() - 1.0f);

        inc[b] = rate * jitter / sr;

        const float t = std::tan (juce::MathConstants<float>::pi
                                  * juce::jlimit (20.0f, sr * 0.45f, bands[b].freq) / sr);
        const float aFreq = (t - 1.0f) / (t + 1.0f);
        aBaseT[b] = juce::jlimit (-0.98f, 0.98f, aFreq * bands[b].phase);
    }

    for (auto& c : channels)
    {
        c.splitLP[0].setLowpass  (cross1, sr);  c.splitHP[0].setHighpass (cross1, sr);
        c.splitLP[1].setLowpass  (cross2, sr);  c.splitHP[1].setHighpass (cross2, sr);
        c.splitLP[2].setLowpass  (cross3, sr);  c.splitHP[2].setHighpass (cross3, sr);
    }

    const float aSmoothK = 1.0f - std::exp (-1.0f / (sr * 0.004f));
    const float dlyK     = 1.0f - std::exp (-1.0f / (sr * 0.050f));

    // ---- per-sample engine -----------------------------------------------
    for (int n = 0; n < numSamples; ++n)
    {
        const float inGain  = smInGain.getNextValue();
        const float outGain = smOutGain.getNextValue();
        const float mixNow  = smMix.getNextValue();
        const float fbNow   = smFeedback.getNextValue();
        const float driveNow= smDrive.getNextValue();
        const float ringNow = smRing.getNextValue();
        const float tiltNow = smTilt.getNextValue();
        const float topoNow = smTopo.getNextValue();

        for (int b = 0; b < pid::numBands; ++b)
            bands[b].aSmooth += aSmoothK * (aBaseT[b] - bands[b].aSmooth);

        float wet[2] = { 0.0f, 0.0f };
        float dry[2] = { 0.0f, 0.0f };

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& c = channels[(size_t) ch];

            const float dryVal = buffer.getSample (ch, n);
            dry[ch] = dryVal;

            float x = freezeV ? 0.0f : dryVal * inGain;
            x += c.fbMemory * fbNow;

            // pre tilt -----------------------------------------------------
            const float lp = c.tiltLP.process (x, tiltK);
            x = lp * (1.0f - tiltNow) + (x - lp) * (1.0f + tiltNow);

            // band split ---------------------------------------------------
            const float s0 = c.splitLP[0].process (x);
            const float h0 = c.splitHP[0].process (x);
            const float s1 = c.splitLP[1].process (h0);
            const float h1 = c.splitHP[1].process (h0);
            const float s2 = c.splitLP[2].process (h1);
            const float h2 = c.splitHP[2].process (h1);
            const float bandSig[4] = { s0, s1, s2, h2 };

            // LFOs ---------------------------------------------------------
            float a[pid::numBands];

            for (int b = 0; b < pid::numBands; ++b)
            {
                const float ph = c.lfoPhase[b]
                               + globalPhaseNorm
                               + spreadAmount * ((float) b * 0.31f + (float) ch * 0.5f);
                const float lfo = lfoValue (ph, waveShape);

                a[b] = juce::jlimit (-0.985f, 0.985f,
                                     bands[b].aSmooth + bands[b].depth * lfo * 0.9f);

                c.lfoPhase[b] += inc[b];
                if (c.lfoPhase[b] >= 1.0f)
                    c.lfoPhase[b] -= 1.0f;
            }

            // per-channel echo settings ------------------------------------
            const float delayBase = smEchoTime[ch].getNextValue();
            const float fbEcho    = smEchoFb[ch].getNextValue() * 0.92f;
            const float mixEcho   = smEchoMix[ch].getNextValue();

            // band allpass chains + per-band echo --------------------------
            float parallel = 0.0f;
            float echoSum  = 0.0f;

            for (int b = 0; b < pid::numBands; ++b)
            {
                float v = bandSig[b];

                for (int s = 0; s < 4; ++s)
                    v = c.bandAP[b][s].process (v, a[b]);

                // each band owns a delay line, so every band gets its own
                // repetition - the ratios are what make the echoes split apart
                auto& tap = c.echo[b];

                const float want = juce::jlimit (1.0f, maxEchoSeconds * sr - 4.0f,
                                                 delayBase * echoBandScale[b]);

                if (tap.delaySamples <= 0.0f)
                    tap.delaySamples = want;
                else
                    tap.delaySamples += (want - tap.delaySamples) * dlyK;

                const float tapped = tap.read (tap.delaySamples);

                tap.dampState += echoDampK[ch] * (tapped - tap.dampState);
                tap.write (juce::jlimit (-8.0f, 8.0f, v + tap.dampState * fbEcho));

                echoSum += tapped;
                parallel += v;
            }

            // series chain over the whole spectrum ---------------------------
            float v2 = x;

            for (int b = 0; b < pid::numBands; ++b)
                for (int s = 0; s < 4; ++s)
                    v2 = c.seriesAP[b][s].process (v2, a[b]);

            const float series = v2;

            float wetSig = series + topoNow * (parallel - series);

            // per-band echo returns, summed and scaled down a little
            wetSig += mixEcho * echoSum * 0.5f;

            // wavefolder ----------------------------------------------------
            if (driveNow > 0.0f)
            {
                const float g = 1.0f + driveNow * 9.0f;
                const float folded = std::sin (wetSig * g * 0.85f) * 0.85f;
                wetSig = wetSig * (1.0f - driveNow * 0.9f) + folded * driveNow;
            }

            // ring mod / tremolo --------------------------------------------
            if (ringNow > 0.0f)
            {
                const float ph = c.lfoPhase[0] + globalPhaseNorm;
                const float lfo = lfoValue (ph, waveShape);
                wetSig *= (1.0f - ringNow) + ringNow * lfo;
            }

            // feedback memory ------------------------------------------------
            c.fbMemory = juce::jlimit (-4.0f, 4.0f, c.fbLP.process (wetSig, toneK));

            wet[ch] = wetSig * outGain;
        }

        // mid / side rotation ------------------------------------------------
        if (numCh >= 2 && (rotSin != 0.0f))
        {
            const float m = 0.5f * (wet[0] + wet[1]);
            const float s = 0.5f * (wet[0] - wet[1]);
            const float mr = m * rotCos - s * rotSin;
            const float sr2 = m * rotSin + s * rotCos;
            wet[0] = mr + sr2;
            wet[1] = mr - sr2;
        }

        // output ---------------------------------------------------------------
        for (int ch = 0; ch < numCh; ++ch)
        {
            float y = mixNow * wet[ch] + (1.0f - mixNow) * dry[ch];

            // gentle DC blocker keeps the feedback loop from drifting
            const float dcOut = y - dcLast[ch] + dcCoeff * dcState[ch];
            dcLast[ch] = y;
            dcState[ch] = dcOut;

            if (! std::isfinite (dcOut))
            {
                dcState[ch] = 0.0f;
                dcLast[ch] = 0.0f;
                y = 0.0f;
            }

            buffer.setSample (ch, n, juce::jlimit (-16.0f, 16.0f, dcOut));
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* PhaseDriftProcessor::createEditor()
{
    return new PhaseDriftEditor (*this);
}

//==============================================================================
void PhaseDriftProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PhaseDriftProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhaseDriftProcessor();
}
