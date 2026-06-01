#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
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
    D7SChannelStripFullAudioProcessor& audioProcessor;

    RackModuleComponent noiseGate { "D7S NoiseGT1" };
    RackModuleComponent eq4k      { "D7S EQ 4K" };
    RackModuleComponent comp76    { "D7S 76" };
    RackModuleComponent comp2a    { "D7S 2A" };
    RackModuleComponent tube      { "D7S Tube" };
    RackModuleComponent esser     { "D7S Esser" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessorEditor)
};
