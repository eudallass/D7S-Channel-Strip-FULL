#include "PluginEditor.h"

D7SChannelStripFullAudioProcessorEditor::D7SChannelStripFullAudioProcessorEditor (D7SChannelStripFullAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible (noiseGate);
    addAndMakeVisible (eq4k);
    addAndMakeVisible (comp76);
    addAndMakeVisible (comp2a);
    addAndMakeVisible (tube);
    addAndMakeVisible (esser);

    setSize (900, 600);
}

D7SChannelStripFullAudioProcessorEditor::~D7SChannelStripFullAudioProcessorEditor()
{
}

void D7SChannelStripFullAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (20, 20, 20));

    g.setColour (juce::Colours::white);
    g.setFont (28.0f);

    g.drawText ("D7S CHANNEL STRIP",
                0, 10,
                getWidth(), 40,
                juce::Justification::centred);
}

void D7SChannelStripFullAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50);

    const int moduleHeight = 70;
    const int spacing = 15;

    noiseGate.setBounds (area.removeFromTop (moduleHeight));
    area.removeFromTop (spacing);

    eq4k.setBounds (area.removeFromTop (moduleHeight));
    area.removeFromTop (spacing);

    comp76.setBounds (area.removeFromTop (moduleHeight));
    area.removeFromTop (spacing);

    comp2a.setBounds (area.removeFromTop (moduleHeight));
    area.removeFromTop (spacing);

    tube.setBounds (area.removeFromTop (moduleHeight));
    area.removeFromTop (spacing);

    esser.setBounds (area.removeFromTop (moduleHeight));
}
