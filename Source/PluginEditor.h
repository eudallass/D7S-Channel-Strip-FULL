#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/RackModuleComponent.h"

class D7SChannelStripFullAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit D7SChannelStripFullAudioProcessorEditor (D7SChannelStripFullAudioProcessor&);
    ~D7SChannelStripFullAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void updateNoiseGT1VisualState();

    D7SChannelStripFullAudioProcessor& audioProcessor;

    RackModuleComponent noiseGate { "D7S NoiseGT1" };
    RackModuleComponent eq4k      { "D7S EQ 4K" };
    RackModuleComponent comp76    { "D7S 76" };
    RackModuleComponent comp2a    { "D7S 2A" };
    RackModuleComponent tube      { "D7S Tube" };
    RackModuleComponent esser     { "D7S Esser" };

    juce::Label noiseSuppressionLabel;
    juce::Slider noiseSuppressionSlider;
    juce::ToggleButton noiseBypassButton;

    std::unique_ptr<SliderAttachment> noiseSuppressionAttachment;
    std::unique_ptr<ButtonAttachment> noiseBypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessorEditor)
};
