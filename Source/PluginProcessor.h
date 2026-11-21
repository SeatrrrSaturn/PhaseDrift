#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/** Parameter identifiers, shared between the processor and the editor. */
namespace pid
{
    inline constexpr auto inGain    = "in_gain";
    inline constexpr auto mix       = "mix";
    inline constexpr auto outGain   = "out_gain";
    inline constexpr auto drive     = "drive";
    inline constexpr auto ring      = "ring";
    inline constexpr auto tilt      = "tilt";
    inline constexpr auto feedback  = "feedback";
    inline constexpr auto fbTone    = "fb_tone";
    inline constexpr auto chaos     = "chaos";
    inline constexpr auto topology  = "topology";
    inline constexpr auto spread    = "spread";
    inline constexpr auto msRotate  = "ms_rot";
    inline constexpr auto lfoSpeed  = "lfo_speed";
    inline constexpr auto lfoWave   = "lfo_wave";
    inline constexpr auto lfoPhase  = "lfo_phase";
    inline constexpr auto freeze    = "freeze";

    inline constexpr int   numBands = 4;
    inline constexpr int   numChans = 2;

    inline const char* bandFreq  (int b) { static const char* v[] = { "b1_freq",  "b2_freq",  "b3_freq",  "b4_freq"  }; return v[b]; }
    inline const char* bandPhase (int b) { static const char* v[] = { "b1_phase", "b2_phase", "b3_phase", "b4_phase" }; return v[b]; }
    inline const char* bandDepth (int b) { static const char* v[] = { "b1_depth", "b2_depth", "b3_depth", "b4_depth" }; return v[b]; }
    inline const char* bandRate  (int b) { static const char* v[] = { "b1_rate",  "b2_rate",  "b3_rate",  "b4_rate"  }; return v[b]; }
    inline const char* bandEcho  (int b) { static const char* v[] = { "b1_echo",  "b2_echo",  "b3_echo",  "b4_echo"  }; return v[b]; }

    // per output channel (0 = left, 1 = right) - the two channels are fully separate
    inline const char* echoTime  (int c) { static const char* v[] = { "echo_time_l", "echo_time_r" }; return v[c]; }
    inline const char* echoFb    (int c) { static const char* v[] = { "echo_fb_l",   "echo_fb_r"   }; return v[c]; }
    inline const char* echoDamp  (int c) { static const char* v[] = { "echo_damp_l", "echo_damp_r" }; return v[c]; }
    inline const char* echoMix   (int c) { static const char* v[] = { "echo_mix_l",  "echo_mix_r"  }; return v[c]; }
}

//==============================================================================
class PhaseDriftProcessor final : public juce::AudioProcessor
{
public:
    PhaseDriftProcessor();
    ~PhaseDriftProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    //==========================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    /** Longest delay the echo taps can reach, in seconds. */
    static constexpr float maxEchoSeconds = 3.2f;

private:
    //==========================================================================
    struct Allpass1
    {
        float z = 0.0f;

        inline float process (float x, float a) noexcept
        {
            const float y = a * x + z;
            z = x - a * y;
            return y;
        }

        void reset() noexcept { z = 0.0f; }
    };

    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float s1 = 0.0f, s2 = 0.0f;

        inline float process (float x) noexcept
        {
            const float y = b0 * x + s1;
            s1 = b1 * x - a1 * y + s2;
            s2 = b2 * x - a2 * y;
            return y;
        }

        void reset() noexcept { s1 = s2 = 0.0f; }

        void setLowpass  (float fc, float sr) noexcept;
        void setHighpass (float fc, float sr) noexcept;
    };

    struct OnePole
    {
        float z = 0.0f;

        inline float process (float x, float k) noexcept
        {
            z += k * (x - z);
            return z;
        }

        void reset() noexcept { z = 0.0f; }
    };

    //==========================================================================
    /** One fractional delay line per band, per channel. */
    struct EchoTap
    {
        std::vector<float> buf;
        int   writePos = 0;
        float delaySamples = 0.0f;
        float dampState = 0.0f;

        void prepare (int maxSamples);
        void reset() noexcept;

        inline float read (float delay) noexcept
        {
            const int n = (int) buf.size();

            if (n < 4)
                return 0.0f;

            const float hi = (float) (n - 2);
            delay = juce::jlimit (1.0f, hi, delay);

            float pos = (float) writePos - delay;
            while (pos < 0.0f)
                pos += (float) n;

            int i0 = (int) pos;
            if (i0 >= n) i0 -= n;

            const float frac = pos - (float) i0;
            const int i1 = (i0 + 1 >= n) ? 0 : i0 + 1;

            const float a = buf[(size_t) i0];
            const float b = buf[(size_t) i1];
            return a + frac * (b - a);
        }

        inline void write (float x) noexcept
        {
            if (buf.empty())
                return;

            buf[(size_t) writePos] = x;

            if (++writePos >= (int) buf.size())
                writePos = 0;
        }
    };

    //==========================================================================
    struct Channel
    {
        Allpass1 bandAP[pid::numBands][4];
        Allpass1 seriesAP[pid::numBands][4];

        Biquad splitLP[3], splitHP[3];

        EchoTap echo[pid::numBands];

        OnePole tiltLP, postLP, fbLP;
        float lfoPhase[pid::numBands] {};
        float fbMemory = 0.0f;

        void reset() noexcept;
    };

    std::vector<Channel> channels;

    //==========================================================================
    // smoothed / per-block values
    float sr = 48000.0f;
    float tiltK = 0.05f, toneK = 0.5f;
    float dcCoeff = 0.995f;
    float rotCos = 1.0f, rotSin = 0.0f;
    float spreadAmount = 0.0f, globalPhaseNorm = 0.0f, waveShape = 0.0f, chaosAmount = 0.0f;

    juce::SmoothedValue<float> smInGain, smOutGain, smMix, smFeedback, smDrive, smRing, smTilt, smTopo;

    // echo: independent per channel
    juce::SmoothedValue<float> smEchoTime[pid::numChans];
    juce::SmoothedValue<float> smEchoFb[pid::numChans];
    juce::SmoothedValue<float> smEchoMix[pid::numChans];
    float echoDampK[pid::numChans] { 0.5f, 0.5f };
    float echoBandScale[pid::numBands] { 1.0f, 1.0f, 1.0f, 1.0f };

    float dcState[2] { 0.0f, 0.0f };
    float dcLast[2] { 0.0f, 0.0f };

    struct BandRuntime
    {
        float freq = 100.0f;
        float phase = 0.0f;
        float depth = 0.0f;
        float rate = 1.0f;
        float aBase = 0.0f;
        float aSmooth = 0.0f;
    };

    BandRuntime bands[pid::numBands];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseDriftProcessor)
};
