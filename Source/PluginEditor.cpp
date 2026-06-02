#include "PluginEditor.h"

namespace
{
    constexpr int designWidth  = 1640;
    constexpr int designHeight = 760;

    static juce::String dbText (float value)
    {
        if (value <= -119.0f) return "-inf dB";
        return juce::String (value, 1) + " dB";
    }

    static float meterNormFromDb (float db, bool gr)
    {
        if (gr) return juce::jlimit (0.0f, 1.0f, db / 24.0f);
        return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    }

    static void layoutKnob (D7SChannelStripFullAudioProcessorEditor::ParamSlider& control,
                            juce::Rectangle<int> r)
    {
        control.label.setBounds (r.removeFromTop (18));
        control.slider.setBounds (r);
    }

    static juce::Rectangle<int> nextRowLeft (juce::Rectangle<int>& row, int width = 76)
    {
        auto r = row.removeFromLeft (width);
        row.removeFromLeft (8);
        return r;
    }
}

void D7SChannelStripFullAudioProcessorEditor::HorizontalMeter::setDbValue (float newDb, bool isGainReduction)
{
    dbValue = newDb;
    grMode = isGainReduction;
    repaint();
}

void D7SChannelStripFullAudioProcessorEditor::HorizontalMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (35, 35, 35));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colour (90, 90, 90));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    auto fill = r.reduced (3.0f);
    fill.setWidth (fill.getWidth() * meterNormFromDb (dbValue, grMode));
    g.setColour (grMode ? juce::Colour (210, 70, 60) : juce::Colour (70, 190, 110));
    g.fillRoundedRectangle (fill, 2.0f);
}

void D7SChannelStripFullAudioProcessorEditor::SpectrumView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    g.setColour (juce::Colour (18, 18, 18));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (juce::Colour (75, 75, 75));
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);

    const int bins = D7SChannelStripFullAudioProcessor::numSpectrumBins;
    const float w = bounds.getWidth() / (float) bins;
    for (int i = 0; i < bins; ++i)
    {
        const float db = processor.getSpectrumBinDb (i);
        const float n = juce::jlimit (0.0f, 1.0f, (db + 80.0f) / 80.0f);
        auto bar = juce::Rectangle<float> (bounds.getX() + i * w + 2.0f,
                                           bounds.getBottom() - n * bounds.getHeight(),
                                           w - 4.0f,
                                           n * bounds.getHeight());
        g.setColour (juce::Colour (90, 160, 220));
        g.fillRoundedRectangle (bar, 2.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.setFont (11.0f);
    g.drawText ("Rack Output Spectrum", getLocalBounds().reduced (8), juce::Justification::topLeft);
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

    for (auto* m : { &noiseGate, &eq4k, &comp76, &comp2a, &tube, &esser }) content.addAndMakeVisible (*m);

    setupSlider (rackInput, "Rack In", "rack_input");
    setupSlider (rackOutput, "Rack Out", "rack_output");
    setupSlider (rackMix, "Rack Mix", "rack_mix");
    rackMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (rackMeterLabel);
    content.addAndMakeVisible (rackInMeter);
    content.addAndMakeVisible (rackOutMeter);
    content.addAndMakeVisible (spectrumView);

    setupSlider (noiseSuppression, "Suppression", "noisegt1_suppression");
    setupBypassButton (noiseBypassButton, "NoiseGT1 Bypass", "noisegt1_bypass", noiseBypassAttachment);
    noiseMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (noiseMeterLabel);
    content.addAndMakeVisible (noiseGrMeter);

    setupSlider (eqHpf, "HPF", "eq4k_hpf"); setupSlider (eqLpf, "LPF", "eq4k_lpf");
    setupSlider (eqLfFreq, "LF Freq", "eq4k_lf_freq"); setupSlider (eqLfGain, "LF Gain", "eq4k_lf_gain");
    setupSlider (eqLmfFreq, "LMF Freq", "eq4k_lmf_freq"); setupSlider (eqLmfGain, "LMF Gain", "eq4k_lmf_gain"); setupSlider (eqLmfQ, "LMF Q", "eq4k_lmf_q");
    setupSlider (eqHmfFreq, "HMF Freq", "eq4k_hmf_freq"); setupSlider (eqHmfGain, "HMF Gain", "eq4k_hmf_gain"); setupSlider (eqHmfQ, "HMF Q", "eq4k_hmf_q");
    setupSlider (eqHfFreq, "HF Freq", "eq4k_hf_freq"); setupSlider (eqHfGain, "HF Gain", "eq4k_hf_gain");
    setupBypassButton (eqLfBellButton, "LF Bell", "eq4k_lf_bell", eqLfBellAttachment);
    setupBypassButton (eqHfBellButton, "HF Bell", "eq4k_hf_bell", eqHfBellAttachment);
    setupBypassButton (eqBypassButton, "EQ IN", "eq4k_bypass", eqBypassAttachment);

    setupSlider (comp76Input, "Input", "comp76_input"); setupSlider (comp76Output, "Output", "comp76_output");
    setupSlider (comp76Attack, "Attack", "comp76_attack"); setupSlider (comp76Release, "Release", "comp76_release");
    setupChoiceButtons (comp76RatioButtons, { "4", "8", "12", "20", "All" }, "comp76_ratio");
    setupBypassButton (comp76BypassButton, "76 Bypass", "comp76_bypass", comp76BypassAttachment);
    comp76MeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (comp76MeterLabel);
    content.addAndMakeVisible (comp76GrMeter);

    setupSlider (comp2aPeak, "Peak Red", "comp2a_peak"); setupSlider (comp2aGain, "Gain", "comp2a_gain");
    setupSlider (comp2aEmphasis, "HF", "comp2a_emphasis"); setupSlider (comp2aMix, "Mix", "comp2a_mix");
    setupChoiceButtons (comp2aModeButtons, { "Comp", "Limit" }, "comp2a_mode");
    setupBypassButton (comp2aBypassButton, "2A Bypass", "comp2a_bypass", comp2aBypassAttachment);
    comp2aMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (comp2aMeterLabel);
    content.addAndMakeVisible (comp2aGrMeter);

    setupSlider (tubeBeauty, "Beauty", "tube_beauty"); setupSlider (tubeBeast, "Beast", "tube_beast");
    setupSlider (tubeSensitivity, "Sensitivity", "tube_sensitivity"); setupSlider (tubeMix, "Mix", "tube_mix");
    setupBypassButton (tubeBypassButton, "Tube Bypass", "tube_bypass", tubeBypassAttachment);

    setupSlider (esserThreshold, "Threshold", "esser_threshold"); setupSlider (esserFreq, "Freq", "esser_freq"); setupSlider (esserRange, "Range", "esser_range");
    setupChoiceButtons (esserModeButtons, { "Wide", "Split" }, "esser_mode");
    setupBypassButton (esserBypassButton, "Esser Bypass", "esser_bypass", esserBypassAttachment);
    esserMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (esserMeterLabel);
    content.addAndMakeVisible (esserGrMeter);

    connectRackButton (noiseGate, "noisegt1_bypass"); connectRackButton (eq4k, "eq4k_bypass"); connectRackButton (comp76, "comp76_bypass");
    connectRackButton (comp2a, "comp2a_bypass"); connectRackButton (tube, "tube_bypass"); connectRackButton (esser, "esser_bypass");

    syncRackVisuals(); syncChoiceButtons(); updateScaleButtonStates();
    startTimerHz (15);
    setUIScale (0.75f);
}

D7SChannelStripFullAudioProcessorEditor::~D7SChannelStripFullAudioProcessorEditor() { stopTimer(); }

void D7SChannelStripFullAudioProcessorEditor::setUIScale (float newScale)
{
    uiScale = juce::jlimit (0.25f, 1.0f, newScale);
    setSize ((int) std::round (designWidth * uiScale), (int) std::round (designHeight * uiScale) + 38);
    updateScaleButtonStates(); resized();
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
    control.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.20f, juce::MathConstants<float>::pi * 2.80f, true);
    content.addAndMakeVisible (control.slider);
    control.attachment = std::make_unique<SliderAttachment> (audioProcessor.getAPVTS(), paramID, control.slider);
}

void D7SChannelStripFullAudioProcessorEditor::setupBypassButton (juce::ToggleButton& button, const juce::String& text, const juce::String& paramID, std::unique_ptr<ButtonAttachment>& attachment)
{
    button.setButtonText (text);
    button.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    content.addAndMakeVisible (button);
    attachment = std::make_unique<ButtonAttachment> (audioProcessor.getAPVTS(), paramID, button);
    button.onClick = [this] { syncRackVisuals(); };
}

void D7SChannelStripFullAudioProcessorEditor::setupChoiceButtons (std::array<juce::TextButton, 5>& buttons, const juce::StringArray& labels, const juce::String& paramID)
{
    for (int i = 0; i < (int) buttons.size(); ++i)
    {
        buttons[(size_t) i].setButtonText (labels[i]); content.addAndMakeVisible (buttons[(size_t) i]);
        buttons[(size_t) i].onClick = [this, paramID, i] { setChoiceValue (paramID, i); };
    }
}

void D7SChannelStripFullAudioProcessorEditor::setupChoiceButtons (std::array<juce::TextButton, 2>& buttons, const juce::StringArray& labels, const juce::String& paramID)
{
    for (int i = 0; i < (int) buttons.size(); ++i)
    {
        buttons[(size_t) i].setButtonText (labels[i]); content.addAndMakeVisible (buttons[(size_t) i]);
        buttons[(size_t) i].onClick = [this, paramID, i] { setChoiceValue (paramID, i); };
    }
}

void D7SChannelStripFullAudioProcessorEditor::setChoiceValue (const juce::String& paramID, int index)
{
    if (auto* p = audioProcessor.getAPVTS().getParameter (paramID))
    {
        const float normalised = p->convertTo0to1 ((float) index);
        p->beginChangeGesture(); p->setValueNotifyingHost (normalised); p->endChangeGesture();
    }
    syncChoiceButtons();
}

void D7SChannelStripFullAudioProcessorEditor::syncChoiceButtons()
{
    auto sync = [this] (auto& buttons, const char* id)
    {
        int active = 0;
        if (auto* v = audioProcessor.getAPVTS().getRawParameterValue (id)) active = (int) std::round (v->load());
        for (int i = 0; i < (int) buttons.size(); ++i)
        {
            buttons[(size_t) i].setClickingTogglesState (false);
            const bool on = (i == active);
            buttons[(size_t) i].setColour (juce::TextButton::buttonColourId, on ? juce::Colour (90, 90, 90) : juce::Colour (32, 32, 32));
            buttons[(size_t) i].setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        }
    };
    sync (comp76RatioButtons, "comp76_ratio"); sync (comp2aModeButtons, "comp2a_mode"); sync (esserModeButtons, "esser_mode");
}

void D7SChannelStripFullAudioProcessorEditor::connectRackButton (RackModuleComponent& module, const juce::String& bypassParamID)
{
    module.onEnabledChanged = [this, bypassParamID] (bool enabled)
    {
        if (auto* param = audioProcessor.getAPVTS().getParameter (bypassParamID))
        {
            param->beginChangeGesture(); param->setValueNotifyingHost (enabled ? 0.0f : 1.0f); param->endChangeGesture();
        }
        syncRackVisuals();
    };
}

void D7SChannelStripFullAudioProcessorEditor::syncRackVisuals()
{
    auto& apvts = audioProcessor.getAPVTS();
    auto syncOne = [&apvts] (RackModuleComponent& module, const char* paramID)
    {
        if (auto* value = apvts.getRawParameterValue (paramID)) module.setEnabledState (value->load() <= 0.5f);
    };
    syncOne (noiseGate, "noisegt1_bypass"); syncOne (eq4k, "eq4k_bypass"); syncOne (comp76, "comp76_bypass");
    syncOne (comp2a, "comp2a_bypass"); syncOne (tube, "tube_bypass"); syncOne (esser, "esser_bypass");
}

void D7SChannelStripFullAudioProcessorEditor::timerCallback()
{
    const auto inDb = audioProcessor.getRackInputPeakDb(); const auto outDb = audioProcessor.getRackOutputPeakDb();
    rackMeterLabel.setText ("Rack In " + dbText (inDb) + " / Out " + dbText (outDb), juce::dontSendNotification);
    rackInMeter.setDbValue (inDb, false); rackOutMeter.setDbValue (outDb, false);
    const auto nGr = audioProcessor.getNoiseGT1GainReductionDb(); noiseMeterLabel.setText ("GR " + juce::String (nGr, 1) + " dB", juce::dontSendNotification); noiseGrMeter.setDbValue (nGr, true);
    const auto c76 = audioProcessor.getComp76GainReductionDb(); comp76MeterLabel.setText ("GR " + juce::String (c76, 1) + " dB", juce::dontSendNotification); comp76GrMeter.setDbValue (c76, true);
    const auto c2a = audioProcessor.getComp2AGainReductionDb(); comp2aMeterLabel.setText ("GR " + juce::String (c2a, 1) + " dB", juce::dontSendNotification); comp2aGrMeter.setDbValue (c2a, true);
    const auto es = audioProcessor.getEsserGainReductionDb(); esserMeterLabel.setText ("GR " + juce::String (es, 1) + " dB", juce::dontSendNotification); esserGrMeter.setDbValue (es, true);
    spectrumView.repaint(); syncChoiceButtons();
}

void D7SChannelStripFullAudioProcessorEditor::layoutModuleControls (juce::Rectangle<int>& area, RackModuleComponent& module, std::initializer_list<ParamSlider*> sliders, juce::ToggleButton& bypassButton)
{
    module.setBounds (area.removeFromTop (56)); area.removeFromTop (4);
    auto controlsArea = area.removeFromTop (90);
    for (auto* control : sliders)
    {
        auto group = controlsArea.removeFromLeft (78);
        control->label.setBounds (group.removeFromTop (18)); control->slider.setBounds (group); controlsArea.removeFromLeft (8);
    }
    bypassButton.setBounds (controlsArea.removeFromLeft (150).withHeight (28)); area.removeFromTop (6);
}

void D7SChannelStripFullAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (20, 20, 20)); g.setColour (juce::Colours::white); g.setFont (20.0f);
    g.drawText ("D7S CHANNEL STRIP", 18, 0, 360, 36, juce::Justification::centredLeft);
    g.setFont (13.0f); g.drawText ("Scale", getWidth() - 260, 0, 54, 36, juce::Justification::centredRight);
}

void D7SChannelStripFullAudioProcessorEditor::resized()
{
    auto top = getLocalBounds().removeFromTop (36).reduced (8, 5);
    auto scaleArea = top.removeFromRight (200);
    scale100Button.setBounds (scaleArea.removeFromLeft (48)); scale75Button.setBounds (scaleArea.removeFromLeft (48));
    scale50Button.setBounds (scaleArea.removeFromLeft (48)); scale25Button.setBounds (scaleArea.removeFromLeft (48));

    content.setBounds (0, 38, designWidth, designHeight); content.setTransform (juce::AffineTransform::scale (uiScale));
    auto area = juce::Rectangle<int> (0, 0, designWidth, designHeight).reduced (18);

    auto header = area.removeFromTop (120);
    auto rackControls = header.removeFromLeft (315);
    for (auto* control : { &rackInput, &rackOutput, &rackMix })
    {
        auto group = rackControls.removeFromLeft (92);
        layoutKnob (*control, group.removeFromTop (90));
        rackControls.removeFromLeft (8);
    }
    auto meterArea = header.removeFromLeft (390);
    rackMeterLabel.setBounds (meterArea.removeFromTop (24));
    rackInMeter.setBounds (meterArea.removeFromTop (18)); meterArea.removeFromTop (8);
    rackOutMeter.setBounds (meterArea.removeFromTop (18));
    spectrumView.setBounds (header.reduced (8, 0));
    area.removeFromTop (10);

    auto modules = area;
    const int gap = 10;
    auto noisePanel = modules.removeFromLeft (180); modules.removeFromLeft (gap);
    auto eqPanel    = modules.removeFromLeft (390); modules.removeFromLeft (gap);
    auto comp76Panel= modules.removeFromLeft (230); modules.removeFromLeft (gap);
    auto comp2aPanel= modules.removeFromLeft (260); modules.removeFromLeft (gap);
    auto tubePanel  = modules.removeFromLeft (250); modules.removeFromLeft (gap);
    auto esserPanel = modules.removeFromLeft (260);

    auto panelHeader = [] (juce::Rectangle<int>& panel, RackModuleComponent& module)
    {
        module.setBounds (panel.removeFromTop (56));
        panel.removeFromTop (8);
    };

    panelHeader (noisePanel, noiseGate);
    layoutKnob (noiseSuppression, noisePanel.removeFromTop (96).withWidth (86));
    noiseBypassButton.setBounds (noisePanel.removeFromTop (28)); noisePanel.removeFromTop (8);
    noiseMeterLabel.setBounds (noisePanel.removeFromTop (20));
    noiseGrMeter.setBounds (noisePanel.removeFromTop (16));

    panelHeader (eqPanel, eq4k);
    auto filterRow = eqPanel.removeFromTop (105);
    layoutKnob (eqHpf, nextRowLeft (filterRow));
    layoutKnob (eqLpf, nextRowLeft (filterRow));
    eqBypassButton.setBounds (filterRow.removeFromLeft (90).withHeight (28));
    eqPanel.removeFromTop (8);

    auto hfRow = eqPanel.removeFromTop (105);
    layoutKnob (eqHfGain, nextRowLeft (hfRow));
    layoutKnob (eqHfFreq, nextRowLeft (hfRow));
    eqHfBellButton.setBounds (hfRow.removeFromLeft (90).withHeight (28));
    eqPanel.removeFromTop (8);

    auto hmfRow = eqPanel.removeFromTop (105);
    layoutKnob (eqHmfGain, nextRowLeft (hmfRow));
    layoutKnob (eqHmfFreq, nextRowLeft (hmfRow));
    layoutKnob (eqHmfQ, nextRowLeft (hmfRow));
    eqPanel.removeFromTop (8);

    auto lmfRow = eqPanel.removeFromTop (105);
    layoutKnob (eqLmfGain, nextRowLeft (lmfRow));
    layoutKnob (eqLmfFreq, nextRowLeft (lmfRow));
    layoutKnob (eqLmfQ, nextRowLeft (lmfRow));
    eqPanel.removeFromTop (8);

    auto lfRow = eqPanel.removeFromTop (105);
    layoutKnob (eqLfGain, nextRowLeft (lfRow));
    layoutKnob (eqLfFreq, nextRowLeft (lfRow));
    eqLfBellButton.setBounds (lfRow.removeFromLeft (90).withHeight (28));

    panelHeader (comp76Panel, comp76);
    auto c76Top = comp76Panel.removeFromTop (105);
    layoutKnob (comp76Input, nextRowLeft (c76Top));
    layoutKnob (comp76Output, nextRowLeft (c76Top));
    auto c76Mid = comp76Panel.removeFromTop (105);
    layoutKnob (comp76Attack, nextRowLeft (c76Mid));
    layoutKnob (comp76Release, nextRowLeft (c76Mid));
    comp76BypassButton.setBounds (comp76Panel.removeFromTop (28)); comp76Panel.removeFromTop (8);
    auto ratioArea = comp76Panel.removeFromTop (30);
    for (auto& b : comp76RatioButtons) { b.setBounds (ratioArea.removeFromLeft (40)); ratioArea.removeFromLeft (4); }
    comp76Panel.removeFromTop (8);
    comp76MeterLabel.setBounds (comp76Panel.removeFromTop (20));
    comp76GrMeter.setBounds (comp76Panel.removeFromTop (16));

    panelHeader (comp2aPanel, comp2a);
    auto c2aTop = comp2aPanel.removeFromTop (105);
    layoutKnob (comp2aPeak, nextRowLeft (c2aTop));
    layoutKnob (comp2aGain, nextRowLeft (c2aTop));
    auto c2aMid = comp2aPanel.removeFromTop (105);
    layoutKnob (comp2aEmphasis, nextRowLeft (c2aMid));
    layoutKnob (comp2aMix, nextRowLeft (c2aMid));
    comp2aBypassButton.setBounds (comp2aPanel.removeFromTop (28)); comp2aPanel.removeFromTop (8);
    auto mode2a = comp2aPanel.removeFromTop (30); for (auto& b : comp2aModeButtons) { b.setBounds (mode2a.removeFromLeft (78)); mode2a.removeFromLeft (6); }
    comp2aPanel.removeFromTop (8);
    comp2aMeterLabel.setBounds (comp2aPanel.removeFromTop (20));
    comp2aGrMeter.setBounds (comp2aPanel.removeFromTop (16));

    panelHeader (tubePanel, tube);
    auto tubeTop = tubePanel.removeFromTop (105);
    layoutKnob (tubeBeauty, nextRowLeft (tubeTop));
    layoutKnob (tubeBeast, nextRowLeft (tubeTop));
    auto tubeMid = tubePanel.removeFromTop (105);
    layoutKnob (tubeSensitivity, nextRowLeft (tubeMid));
    layoutKnob (tubeMix, nextRowLeft (tubeMid));
    tubeBypassButton.setBounds (tubePanel.removeFromTop (28));

    panelHeader (esserPanel, esser);
    auto esTop = esserPanel.removeFromTop (105);
    layoutKnob (esserThreshold, nextRowLeft (esTop));
    layoutKnob (esserFreq, nextRowLeft (esTop));
    auto esMid = esserPanel.removeFromTop (105);
    layoutKnob (esserRange, nextRowLeft (esMid));
    esserBypassButton.setBounds (esMid.removeFromLeft (120).withHeight (28));
    auto modeEsser = esserPanel.removeFromTop (30); for (auto& b : esserModeButtons) { b.setBounds (modeEsser.removeFromLeft (82)); modeEsser.removeFromLeft (6); }
    esserPanel.removeFromTop (8);
    esserMeterLabel.setBounds (esserPanel.removeFromTop (20));
    esserGrMeter.setBounds (esserPanel.removeFromTop (16));
}
