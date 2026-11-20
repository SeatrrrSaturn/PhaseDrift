#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/** A dark, hardware-inspired knob. Everything is drawn with vectors, no images. */
class PhaseLAF final : public juce::LookAndFeel_V4
{
public:
    PhaseLAF();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    static juce::Colour accentOf (const juce::Slider& s);
};

//==============================================================================
/** Knob + name + live value readout. */
class LabelledKnob final : public juce::Component
{
public:
    LabelledKnob (const juce::String& name, juce::Colour accent, bool bipolar);

    juce::Slider& getSlider() noexcept { return slider; }

    void paint (juce::Graphics&) override;
    void resized() override;

    void setCompact (bool shouldBeCompact);

private:
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::String label;
    juce::Colour accentColour;
    bool compact = false;
};
