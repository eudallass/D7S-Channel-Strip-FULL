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

    setupSlider (noiseSuppression, "Suppression", "noisegt1_suppression");
    setupBypassButton (noiseBypassButton, "NoiseGT1 Bypass", "noisegt1_bypass", noiseBypassAttachment);

    setupSlider (eqLow, "Low", "eq4k_low");
    setupSlider (eqLowMid, "Low Mid", "eq4k_lowmid");
    setupSlider (eqPresence, "Presence", "eq4k_presence");
    setupSlider (eqAir, "Air", "eq4k_air");
    setupBypassButton (eqBypassButton, "EQ Bypass", "eq4k_bypass", eqBypassAttachment);

    setupSlider (comp76Input, "Input", "comp76_input");
    setupSlider (comp76Attack, "Attack", "comp76_attack");
    setupSlider (comp76Release, "Release", "comp76_release");
    setupSlider (comp76Output, "Output", "comp76_output");
    setupBypassButton (comp76BypassButton, "76 Bypass", "comp76_bypass", comp76BypassAttachment);

    setupSlider (comp2aPeak, "Peak Red", "comp2a_peak");
    setupSlider (comp2aGain, "Gain", "comp2a_gain");
    setupBypassButton (comp2aBypassButton, "2A Bypass", "comp2a_bypass", comp2aBypassAttachment);

    setupSlider (tubeDrive, "Drive", "tube_drive");
    setupSlider (tubeTone, "Tone", "tube_tone");
    setupBypassButton (tubeBypassButton, "Tube Bypass", "tube_bypass", tubeBypassAttachment);

    setupSlider (esserFreq, "Freq", "esser_freq");
    setupSlider (esserAmount, "Amount", "esser_amount");
    setupBypassButton (esserBypassButton, "Esser Bypass", "esser_bypass", esserBypassAttachment);

    connectRackButton (noiseGate, "noisegt1_bypass");
    connectRackButton (eq4k, "eq4k_bypass");
    connectRackButton (comp76, "comp76_bypass");
    connectRackButton (comp2a, "comp2a_bypass");
    connectRackButton (tube, "tube_bypass");
    connectRackButton (esser, "esser_bypass");

    syncRackVisuals();

    setSize (980, 920);
}

D7SChannelStripFullAudioProcessorEditor::~D7SChannelStripFullAudioProcessorEditor()
{
}

void D7SChannelStripFullAudioProcessorEditor::setupSlider (ParamSlider& control, const juce::String& labelText, const juce::String& paramID)
{
    control.label.setText (labelText, juce::dontSendNotification);
    control.label.setJustificationType (juce::Justification::centredRight);
    control.label.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (control.label);

    control.slider.setSliderStyle (juce::Slider::LinearHorizontal);
    control.slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 62, 22);
    addAndMakeVisible (control.slider);

    control.attachment = std::make_unique<SliderAttachment> (audioProcessor.getAPVTS(), paramID, control.slider);
}

void D7SChannelStripFullAudioProcessorEditor::setupBypassButton (juce::ToggleButton& button, const juce::String& text, const juce::String& paramID, std::unique_ptr<ButtonAttachment>& attachment)
{
    button.setButtonText (text);
    button.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible (button);
    attachment = std::make_unique<ButtonAttachment> (audioProcessor.getAPVTS(), paramID, button);
    button.onClick = [this] { syncRackVisuals(); };
}

void D7SChannelStripFullAudioProcessorEditor::connectRackButton (RackModuleComponent& module, const juce::String& bypassParamID)
{
    module.onEnabledChanged = [this, bypassParamID] (bool enabled)
    {
        if (auto* param = audioProcessor.getAPVTS().getParameter (bypassParamID))
        {
            const float value = enabled ? 0.0f : 1.0f;
            param->beginChangeGesture();
            param->setValueNotifyingHost (value);
            param->endChangeGesture();
        }
        syncRackVisuals();
    };
}

void D7SChannelStripFullAudioProcessorEditor::syncRackVisuals()
{
    auto& apvts = audioProcessor.getAPVTS();
    auto syncOne = [&apvts] (RackModuleComponent& module, const char* paramID)
    {
        if (auto* value = apvts.getRawParameterValue (paramID))
            module.setEnabledState (value->load() <= 0.5f);
    };

    syncOne (noiseGate, "noisegt1_bypass");
    syncOne (eq4k, "eq4k_bypass");
    syncOne (comp76, "comp76_bypass");
    syncOne (comp2a, "comp2a_bypass");
    syncOne (tube, "tube_bypass");
    syncOne (esser, "esser_bypass");
}

void D7SChannelStripFullAudioProcessorEditor::layoutModuleControls (juce::Rectangle<int>& area, RackModuleComponent& module, std::initializer_list<ParamSlider*> sliders, juce::ToggleButton& bypassButton)
{
    module.setBounds (area.removeFromTop (62));
    area.removeFromTop (6);

    auto controlsArea = area.removeFromTop (38);
    for (auto* control : sliders)
    {
        auto group = controlsArea.removeFromLeft (210);
        control->label.setBounds (group.removeFromLeft (72));
        control->slider.setBounds (group);
    }

    bypassButton.setBounds (controlsArea.removeFromLeft (180));
    area.removeFromTop (12);
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

    layoutModuleControls (area, noiseGate, { &noiseSuppression }, noiseBypassButton);
    layoutModuleControls (area, eq4k, { &eqLow, &eqLowMid, &eqPresence, &eqAir }, eqBypassButton);
    layoutModuleControls (area, comp76, { &comp76Input, &comp76Attack, &comp76Release, &comp76Output }, comp76BypassButton);
    layoutModuleControls (area, comp2a, { &comp2aPeak, &comp2aGain }, comp2aBypassButton);
    layoutModuleControls (area, tube, { &tubeDrive, &tubeTone }, tubeBypassButton);
    layoutModuleControls (area, esser, { &esserFreq, &esserAmount }, esserBypassButton);
}
