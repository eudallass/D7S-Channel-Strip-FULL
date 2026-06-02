#include "PluginEditor.h"

namespace
{
    constexpr int designWidth  = 1180;
    constexpr int designHeight = 1120;

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
    addAndMakeVisible (content);
    addAndMakeVisible (scale100Button);
    addAndMakeVisible (scale75Button);
    addAndMakeVisible (scale50Button);
    addAndMakeVisible (scale25Button);

    scale100Button.onClick = [this] { setUIScale (1.00f); };
    scale75Button .onClick = [this] { setUIScale (0.75f); };
    scale50Button .onClick = [this] { setUIScale (0.50f); };
    scale25Button .onClick = [this] { setUIScale (0.25f); };

    content.addAndMakeVisible (noiseGate);
    content.addAndMakeVisible (eq4k);
    content.addAndMakeVisible (comp76);
    content.addAndMakeVisible (comp2a);
    content.addAndMakeVisible (tube);
    content.addAndMakeVisible (esser);

    setupSlider (rackInput, "Rack In", "rack_input");
    setupSlider (rackOutput, "Rack Out", "rack_output");
    setupSlider (rackMix, "Rack Mix", "rack_mix");
    rackMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    rackMeterLabel.setJustificationType (juce::Justification::centredLeft);
    content.addAndMakeVisible (rackMeterLabel);

    setupSlider (noiseSuppression, "Suppression", "noisegt1_suppression");
    setupBypassButton (noiseBypassButton, "NoiseGT1 Bypass", "noisegt1_bypass", noiseBypassAttachment);
    noiseMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (noiseMeterLabel);

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
    content.addAndMakeVisible (comp76MeterLabel);

    setupSlider (comp2aPeak, "Peak Red", "comp2a_peak");
    setupSlider (comp2aGain, "Gain", "comp2a_gain");
    setupSlider (comp2aEmphasis, "HF", "comp2a_emphasis");
    setupSlider (comp2aMix, "Mix", "comp2a_mix");
    setupChoice (comp2aMode, "Mode", "comp2a_mode", { "Compress", "Limit" });
    setupBypassButton (comp2aBypassButton, "2A Bypass", "comp2a_bypass", comp2aBypassAttachment);
    comp2aMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (comp2aMeterLabel);

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
    content.addAndMakeVisible (esserMeterLabel);

    connectRackButton (noiseGate, "noisegt1_bypass");
    connectRackButton (eq4k, "eq4k_bypass");
    connectRackButton (comp76, "comp76_bypass");
    connectRackButton (comp2a, "comp2a_bypass");
    connectRackButton (tube, "tube_bypass");
    connectRackButton (esser, "esser_bypass");

    syncRackVisuals();
    updateScaleButtonStates();
    startTimerHz (12);

    setUIScale (0.75f);
}

D7SChannelStripFullAudioProcessorEditor::~D7SChannelStripFullAudioProcessorEditor()
{
    stopTimer();
}

void D7SChannelStripFullAudioProcessorEditor::setUIScale (float newScale)
{
    uiScale = juce::jlimit (0.25f, 1.0f, newScale);
    setSize ((int) std::round (designWidth * uiScale), (int) std::round (designHeight * uiScale) + 38);
    updateScaleButtonStates();
    resized();
}

void D7SChannelStripFullAudioProcessorEditor::updateScaleButtonStates()
{
    scale100Button.setToggleState (std::abs (uiScale - 1.00f) < 0.01f, juce::dontSendNotification);
    scale75Button .setToggleState (std::abs (uiScale - 0.75f) < 0.01f, juce::dontSendNotification);
    scale50Button .setToggleState (std::abs (uiScale - 0.50f) < 0.01f, juce::dontSendNotification);
    scale25Button .setToggleState (std::abs (uiScale - 0.25f) < 0.01f, juce::dontSendNotification);

    for (auto* b : { &scale100Button, &scale75Button, &scale50Button, &scale25Button })
    {
        b->setClickingTogglesState (false);
        b->setColour (juce::TextButton::buttonColourId, b->getToggleState() ? juce::Colour (80, 80, 80) : juce::Colour (35, 35, 35));
        b->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        b->setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }
}

void D7SChannelStripFullAudioProcessorEditor::setupSlider (ParamSlider& control, const juce::String& labelText, const juce::String& paramID)
{
    control.label.setText (labelText, juce::dontSendNotification);
    control.label.setJustificationType (juce::Justification::centred);
    control.label.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (control.label);

    control.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    control.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
    control.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.20f,
                                        juce::MathConstants<float>::pi * 2.80f,
                                        true);
    content.addAndMakeVisible (control.slider);

    control.attachment = std::make_unique<SliderAttachment> (audioProcessor.getAPVTS(), paramID, control.slider);
}

void D7SChannelStripFullAudioProcessorEditor::setupChoice (ParamChoice& control, const juce::String& labelText, const juce::String& paramID, const juce::StringArray& items)
{
    control.label.setText (labelText, juce::dontSendNotification);
    control.label.setJustificationType (juce::Justification::centred);
    control.label.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (control.label);

    control.box.addItemList (items, 1);
    content.addAndMakeVisible (control.box);
    control.attachment = std::make_unique<ComboBoxAttachment> (audioProcessor.getAPVTS(), paramID, control.box);
}

void D7SChannelStripFullAudioProcessorEditor::setupBypassButton (juce::ToggleButton& button, const juce::String& text, const juce::String& paramID, std::unique_ptr<ButtonAttachment>& attachment)
{
    button.setButtonText (text);
    button.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    content.addAndMakeVisible (button);
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
    area.removeFromTop (4);

    auto controlsArea = area.removeFromTop (92);
    for (auto* control : sliders)
    {
        auto group = controlsArea.removeFromLeft (78);
        control->label.setBounds (group.removeFromTop (18));
        control->slider.setBounds (group);
        controlsArea.removeFromLeft (8);
    }

    for (auto* control : choices)
    {
        auto group = controlsArea.removeFromLeft (95);
        control->label.setBounds (group.removeFromTop (18));
        control->box.setBounds (group.removeFromTop (24));
        controlsArea.removeFromLeft (8);
    }

    bypassButton.setBounds (controlsArea.removeFromLeft (150).withHeight (28));
    area.removeFromTop (6);
}

void D7SChannelStripFullAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (20, 20, 20));
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("D7S CHANNEL STRIP", 18, 0, 360, 36, juce::Justification::centredLeft);
    g.setFont (13.0f);
    g.drawText ("Scale", getWidth() - 260, 0, 54, 36, juce::Justification::centredRight);
}

void D7SChannelStripFullAudioProcessorEditor::resized()
{
    auto top = getLocalBounds().removeFromTop (36).reduced (8, 5);
    auto scaleArea = top.removeFromRight (200);
    scale100Button.setBounds (scaleArea.removeFromLeft (48));
    scale75Button .setBounds (scaleArea.removeFromLeft (48));
    scale50Button .setBounds (scaleArea.removeFromLeft (48));
    scale25Button .setBounds (scaleArea.removeFromLeft (48));

    content.setBounds (0, 38, designWidth, designHeight);
    content.setTransform (juce::AffineTransform::scale (uiScale));

    auto area = juce::Rectangle<int> (0, 0, designWidth, designHeight).reduced (18);
    area.removeFromTop (8);

    auto rackArea = area.removeFromTop (102);
    for (auto* control : { &rackInput, &rackOutput, &rackMix })
    {
        auto group = rackArea.removeFromLeft (86);
        control->label.setBounds (group.removeFromTop (18));
        control->slider.setBounds (group.removeFromTop (76));
        rackArea.removeFromLeft (8);
    }
    rackMeterLabel.setBounds (rackArea.removeFromLeft (420).withHeight (28));
    area.removeFromTop (8);

    layoutModuleControls (area, noiseGate, { &noiseSuppression }, {}, noiseBypassButton);
    noiseMeterLabel.setBounds (area.removeFromTop (20)); area.removeFromTop (4);

    layoutModuleControls (area, eq4k, { &eqHpf, &eqLpf, &eqLfFreq, &eqLfGain, &eqLmfFreq, &eqLmfGain, &eqLmfQ, &eqHmfFreq, &eqHmfGain, &eqHmfQ, &eqHfFreq, &eqHfGain }, {}, eqBypassButton);
    auto eqButtons = area.removeFromTop (30);
    eqLfBellButton.setBounds (eqButtons.removeFromLeft (110));
    eqHfBellButton.setBounds (eqButtons.removeFromLeft (110));
    area.removeFromTop (6);

    layoutModuleControls (area, comp76, { &comp76Input, &comp76Output, &comp76Attack, &comp76Release }, { &comp76Ratio }, comp76BypassButton);
    comp76MeterLabel.setBounds (area.removeFromTop (20)); area.removeFromTop (4);

    layoutModuleControls (area, comp2a, { &comp2aPeak, &comp2aGain, &comp2aEmphasis, &comp2aMix }, { &comp2aMode }, comp2aBypassButton);
    comp2aMeterLabel.setBounds (area.removeFromTop (20)); area.removeFromTop (4);

    layoutModuleControls (area, tube, { &tubeBeauty, &tubeBeast, &tubeSensitivity }, {}, tubeBypassButton);

    layoutModuleControls (area, esser, { &esserThreshold, &esserFreq, &esserRange }, { &esserMode }, esserBypassButton);
    esserMeterLabel.setBounds (area.removeFromTop (20));
}
