#include "PluginEditor.h"

D7SChannelStripFullAudioProcessorEditor::D7SChannelStripFullAudioProcessorEditor (D7SChannelStripFullAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (800, 400);
}

D7SChannelStripFullAudioProcessorEditor::~D7SChannelStripFullAudioProcessorEditor()
{
}

void D7SChannelStripFullAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    g.setColour (juce::Colours::white);
    g.setFont (30.0f);

    g.drawFittedText (
        "D7S Channel Strip FULL",
        getLocalBounds(),
        juce::Justification::centred,
        1
    );
}

void D7SChannelStripFullAudioProcessorEditor::resized()
{
}
