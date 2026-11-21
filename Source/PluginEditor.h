#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PhaseLookAndFeel.h"

//==============================================================================
class PhaseDriftEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PhaseDriftEditor (PhaseDriftProcessor&);
    ~PhaseDriftEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    LabelledKnob& addKnob (const char* paramId, const char* name,
                           juce::Colour accent, bool bipolar, bool compact);

    void applyPreset (int presetIndex);

    PhaseDriftProcessor& processor;
    PhaseLAF laf;

    std::vector<std::unique_ptr<LabelledKnob>> knobs;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;

    juce::ComboBox presetBox;
    juce::TextButton freezeButton { "FREEZE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;

    juce::Rectangle<int> bandRects[pid::numBands];
    juce::Rectangle<int> bandEchoRect, coreRect, motionRect, echoLeftRect, echoRightRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseDriftEditor)
};
