#include "PluginEditor.h"

D7SChannelStripFullAudioProcessorEditor::D7SChannelStripFullAudioProcessorEditor (D7SChannelStripFullAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
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

    const char* modules[] =
    {
        "D7S NoiseGT1",
        "D7S EQ 4K",
        "D7S 76",
        "D7S 2A",
        "D7S Tube",
        "D7S Esser"
    };

    int y = 70;

    for (auto* module : modules)
    {
        g.setColour (juce::Colour (45, 45, 45));
        g.fillRoundedRectangle (20.0f, (float) y, 860.0f, 70.0f, 8.0f);

        g.setColour (juce::Colours::silver);
        g.drawRoundedRectangle (20.0f, (float) y, 860.0f, 70.0f, 8.0f, 2.0f);

        g.setColour (juce::Colours::white);
        g.setFont (22.0f);

        g.drawText (module,
                    40,
                    y,
                    400,
                    70,
                    juce::Justification::centredLeft);

        y += 85;
    }
}

void D7SChannelStripFullAudioProcessorEditor::resized()
{
}
