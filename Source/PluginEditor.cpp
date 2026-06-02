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

    noiseSuppressionLabel.setText ("Suppression", juce::dontSendNotification);
    noiseSuppressionLabel.setJustificationType (juce::Justification::centredRight);
    noiseSuppressionLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (noiseSuppressionLabel);

    noiseSuppressionSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    noiseSuppressionSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 24);
    noiseSuppressionSlider.setRange (0.0, 1.0, 0.01);
    addAndMakeVisible (noiseSuppressionSlider);

    noiseBypassButton.setButtonText ("NoiseGT1 Bypass");
    noiseBypassButton.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible (noiseBypassButton);

    auto& apvts = audioProcessor.getAPVTS();
    noiseSuppressionAttachment = std::make_unique<SliderAttachment> (apvts, "noisegt1_suppression", noiseSuppressionSlider);
    noiseBypassAttachment = std::make_unique<ButtonAttachment> (apvts, "noisegt1_bypass", noiseBypassButton);

    noiseGate.onEnabledChanged = [this] (bool enabled)
    {
        if (auto* param = audioProcessor.getAPVTS().getParameter ("noisegt1_bypass"))
        {
            const float value = enabled ? 0.0f : 1.0f;
            param->beginChangeGesture();
            param->setValueNotifyingHost (value);
            param->endChangeGesture();
        }
    };

    noiseBypassButton.onClick = [this]
    {
        updateNoiseGT1VisualState();
    };

    updateNoiseGT1VisualState();

    setSize (900, 640);
}

D7SChannelStripFullAudioProcessorEditor::~D7SChannelStripFullAudioProcessorEditor()
{
}

void D7SChannelStripFullAudioProcessorEditor::updateNoiseGT1VisualState()
{
    const bool bypassed = noiseBypassButton.getToggleState();
    noiseGate.setEnabledState (! bypassed);
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
    area.removeFromTop (8);

    auto noiseControlsArea = area.removeFromTop (42);
    noiseSuppressionLabel.setBounds (noiseControlsArea.removeFromLeft (120));
    noiseSuppressionSlider.setBounds (noiseControlsArea.removeFromLeft (470));
    noiseControlsArea.removeFromLeft (20);
    noiseBypassButton.setBounds (noiseControlsArea.removeFromLeft (180));

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
