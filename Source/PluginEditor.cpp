#include "PluginEditor.h"

namespace
{
    static juce::String dbText (float value)
    {
        if (value <= -119.0f)
            return "-inf dB";
        return juce::String (value, 1) + " dB";
    }
}

D7SChannelStripFullAudioProcessorEditor::D7SChannelStripFullAudioProcessorEditor (D7SChannelStripFullAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible (noiseGate);
    addAndMakeVisible (eq4k);
    addAndMakeVisible (comp76);
    addAndMakeVisible (comp2a);
    addAndMakeVisible (tube);
    addAndMakeVisible (esser);

    setupSlider (rackInput, "Rack In", "rack_input");
    setupSlider (rackOutput, "Rack Out", "rack_output");
    setupSlider (rackMix, "Rack Mix", "rack_mix");
    rackMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    rackMeterLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (rackMeterLabel);

    setupSlider (noiseSuppression, "Suppression", "noisegt1_suppression");
    setupBypassButton (noiseBypassButton, "NoiseGT1 Bypass", "noisegt1_bypass", noiseBypassAttachment);
    noiseMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (noiseMeterLabel);

    setupSlider (eqHpf, "HPF", "eq4k_hpf");
    setupSlider (eqLpf, "LPF", "eq4k_lpf");
    setupSlider (eqLfFreq, "LF Freq", "eq4k_lf_freq");
    setupSlider (eqLfGain, "LF Gain", "eq4k_lf_gain");
    setupSlider (eqLmfFreq, "LMF Freq", "eq4k_lmf_freq");
    setupSlider (eqLmfGain, "LMF Gain", "eq4k_lmf_gain");
    setupSlider (eqLmfQ, "LMF Q", "eq4k_lmf_q");
    setupSlider (eqHmfFreq, "HMF Freq", "eq4k_hmf_freq");
    setupSlider (eqHmfGain, "HMF Gain", "eq4k_hmf_gain");
    setupSlider (eqHmfQ, "HMF Q", "eq4k_hmf_q");
    setupSlider (eqHfFreq, "HF Freq", "eq4k_hf_freq");
    setupSlider (eqHfGain, "HF Gain", "eq4k_hf_gain");
    setupBypassButton (eqLfBellButton, "LF Bell", "eq4k_lf_bell", eqLfBellAttachment);
    setupBypassButton (eqHfBellButton, "HF Bell", "eq4k_hf_bell", eqHfBellAttachment);
    setupBypassButton (eqBypassButton, "EQ Bypass", "eq4k_bypass", eqBypassAttachment);

    setupSlider (comp76Input, "Input", "comp76_input");
    setupSlider (comp76Output, "Output", "comp76_output");
    setupSlider (comp76Attack, "Attack", "comp76_attack");
    setupSlider (comp76Release, "Release", "comp76_release");
    setupChoice (comp76Ratio, "Ratio", "comp76_ratio", { "4", "8", "12", "20", "All" });
    setupBypassButton (comp76BypassButton, "76 Bypass", "comp76_bypass", comp76BypassAttachment);
    comp76MeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (comp76MeterLabel);

    setupSlider (comp2aPeak, "Peak Red", "comp2a_peak");
    setupSlider (comp2aGain, "Gain", "comp2a_gain");
    setupSlider (comp2aEmphasis, "HF", "comp2a_emphasis");
    setupSlider (comp2aMix, "Mix", "comp2a_mix");
    setupChoice (comp2aMode, "Mode", "comp2a_mode", { "Compress", "Limit" });
    setupBypassButton (comp2aBypassButton, "2A Bypass", "comp2a_bypass", comp2aBypassAttachment);
    comp2aMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (comp2aMeterLabel);

    setupSlider (tubeBeauty, "Beauty", "tube_beauty");
    setupSlider (tubeBeast, "Beast", "tube_beast");
    setupSlider (tubeSensitivity, "Sensitivity", "tube_sensitivity");
    setupBypassButton (tubeBypassButton, "Tube Bypass", "tube_bypass", tubeBypassAttachment);

    setupSlider (esserThreshold, "Threshold", "esser_threshold");
    setupSlider (esserFreq, "Freq", "esser_freq");
    setupSlider (esserRange, "Range", "esser_range");
    setupChoice (esserMode, "Mode", "esser_mode", { "Wide", "Split" });
    setupBypassButton (esserBypassButton, "Esser Bypass", "esser_bypass", esserBypassAttachment);
    esserMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (esserMeterLabel);

    connectRackButton (noiseGate, "noisegt1_bypass");
    connectRackButton (eq4k, "eq4k_bypass");
    connectRackButton (comp76, "comp76_bypass");
    connectRackButton (comp2a, "comp2a_bypass");
    connectRackButton (tube, "tube_bypass");
    connectRackButton (esser, "esser_bypass");

    syncRackVisuals();
    startTimerHz (12);

    setSize (1180, 1120);
}

D7SChannelStripFullAudioProcessorEditor::~D7SChannelStripFullAudioProcessorEditor()
{
    stopTimer();
}

void D7SChannelStripFullAudioProcessorEditor::setupSlider (ParamSlider& control, const juce::String& labelText, const juce::String& paramID)
{
    control.label.setText (labelText, juce::dontSendNotification);
    control.label.setJustificationType (juce::Justification::centredRight);
    control.label.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (control.label);

    control.slider.setSliderStyle (juce::Slider::LinearHorizontal);
    control.slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 72, 22);
    addAndMakeVisible (control.slider);

    control.attachment = std::make_unique<SliderAttachment> (audioProcessor.getAPVTS(), paramID, control.slider);
}

void D7SChannelStripFullAudioProcessorEditor::setupChoice (ParamChoice& control, const juce::String& labelText, const juce::String& paramID, const juce::StringArray& items)
{
    control.label.setText (labelText, juce::dontSendNotification);
    control.label.setJustificationType (juce::Justification::centredRight);
    control.label.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (control.label);

    control.box.addItemList (items, 1);
    addAndMakeVisible (control.box);
    control.attachment = std::make_unique<ComboBoxAttachment> (audioProcessor.getAPVTS(), paramID, control.box);
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

void D7SChannelStripFullAudioProcessorEditor::timerCallback()
{
    rackMeterLabel.setText ("Rack In " + dbText (audioProcessor.getRackInputPeakDb()) + " / Out " + dbText (audioProcessor.getRackOutputPeakDb()), juce::dontSendNotification);
    noiseMeterLabel.setText ("GR " + juce::String (audioProcessor.getNoiseGT1GainReductionDb(), 1) + " dB", juce::dontSendNotification);
    comp76MeterLabel.setText ("GR " + juce::String (audioProcessor.getComp76GainReductionDb(), 1) + " dB", juce::dontSendNotification);
    comp2aMeterLabel.setText ("GR " + juce::String (audioProcessor.getComp2AGainReductionDb(), 1) + " dB", juce::dontSendNotification);
    esserMeterLabel.setText ("GR " + juce::String (audioProcessor.getEsserGainReductionDb(), 1) + " dB", juce::dontSendNotification);
}

void D7SChannelStripFullAudioProcessorEditor::layoutModuleControls (juce::Rectangle<int>& area, RackModuleComponent& module, std::initializer_list<ParamSlider*> sliders, std::initializer_list<ParamChoice*> choices, juce::ToggleButton& bypassButton)
{
    module.setBounds (area.removeFromTop (58));
    area.removeFromTop (5);

    auto controlsArea = area.removeFromTop (32);
    for (auto* control : sliders)
    {
        auto group = controlsArea.removeFromLeft (178);
        control->label.setBounds (group.removeFromLeft (72));
        control->slider.setBounds (group);
    }

    for (auto* control : choices)
    {
        auto group = controlsArea.removeFromLeft (155);
        control->label.setBounds (group.removeFromLeft (55));
        control->box.setBounds (group.reduced (0, 4));
    }

    bypassButton.setBounds (controlsArea.removeFromLeft (160));
    area.removeFromTop (10);
}

void D7SChannelStripFullAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (20, 20, 20));
    g.setColour (juce::Colours::white);
    g.setFont (28.0f);
    g.drawText ("D7S CHANNEL STRIP", 0, 10, getWidth(), 40, juce::Justification::centred);
}

void D7SChannelStripFullAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    area.removeFromTop (48);

    auto rackArea = area.removeFromTop (42);
    rackInput.label.setBounds (rackArea.removeFromLeft (70)); rackInput.slider.setBounds (rackArea.removeFromLeft (160));
    rackOutput.label.setBounds (rackArea.removeFromLeft (80)); rackOutput.slider.setBounds (rackArea.removeFromLeft (160));
    rackMix.label.setBounds (rackArea.removeFromLeft (75)); rackMix.slider.setBounds (rackArea.removeFromLeft (160));
    rackMeterLabel.setBounds (rackArea.removeFromLeft (360));
    area.removeFromTop (8);

    layoutModuleControls (area, noiseGate, { &noiseSuppression }, {}, noiseBypassButton);
    noiseMeterLabel.setBounds (area.removeFromTop (20)); area.removeFromTop (4);

    layoutModuleControls (area, eq4k, { &eqHpf, &eqLpf, &eqLfFreq, &eqLfGain, &eqLmfFreq, &eqLmfGain }, {}, eqBypassButton);
    auto eqRow2 = area.removeFromTop (32);
    eqLmfQ.label.setBounds (eqRow2.removeFromLeft (70)); eqLmfQ.slider.setBounds (eqRow2.removeFromLeft (110));
    eqHmfFreq.label.setBounds (eqRow2.removeFromLeft (80)); eqHmfFreq.slider.setBounds (eqRow2.removeFromLeft (110));
    eqHmfGain.label.setBounds (eqRow2.removeFromLeft (80)); eqHmfGain.slider.setBounds (eqRow2.removeFromLeft (110));
    eqHmfQ.label.setBounds (eqRow2.removeFromLeft (70)); eqHmfQ.slider.setBounds (eqRow2.removeFromLeft (110));
    eqHfFreq.label.setBounds (eqRow2.removeFromLeft (80)); eqHfFreq.slider.setBounds (eqRow2.removeFromLeft (110));
    eqHfGain.label.setBounds (eqRow2.removeFromLeft (80)); eqHfGain.slider.setBounds (eqRow2.removeFromLeft (110));
    eqLfBellButton.setBounds (eqRow2.removeFromLeft (90));
    eqHfBellButton.setBounds (eqRow2.removeFromLeft (90));
    area.removeFromTop (8);

    layoutModuleControls (area, comp76, { &comp76Input, &comp76Output, &comp76Attack, &comp76Release }, { &comp76Ratio }, comp76BypassButton);
    comp76MeterLabel.setBounds (area.removeFromTop (20)); area.removeFromTop (4);

    layoutModuleControls (area, comp2a, { &comp2aPeak, &comp2aGain, &comp2aEmphasis, &comp2aMix }, { &comp2aMode }, comp2aBypassButton);
    comp2aMeterLabel.setBounds (area.removeFromTop (20)); area.removeFromTop (4);

    layoutModuleControls (area, tube, { &tubeBeauty, &tubeBeast, &tubeSensitivity }, {}, tubeBypassButton);

    layoutModuleControls (area, esser, { &esserThreshold, &esserFreq, &esserRange }, { &esserMode }, esserBypassButton);
    esserMeterLabel.setBounds (area.removeFromTop (20));
}
