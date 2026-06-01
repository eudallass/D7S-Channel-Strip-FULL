#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class D7SChannelStripFullAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit D7SChannelStripFullAudioProcessorEditor (D7SChannelStripFullAudioProcessor&);
    ~D7SChannelStripFullAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    D7SChannelStripFullAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessorEditor)
};
