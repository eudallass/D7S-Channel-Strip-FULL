#include "PluginEditor.h"

namespace
{
    constexpr int designWidth  = 2040;
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

    static juce::Rectangle<int> nextRowLeft (juce::Rectangle<int>& row, int width = 76)
    {
        auto r = row.removeFromLeft (width);
        row.removeFromLeft (8);
        return r;
    }

    static void setHostParamValue (juce::AudioProcessorValueTreeState& apvts, const char* id, float value)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
        }
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
    g.setColour (juce::Colour (10, 12, 15));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (juce::Colour (65, 72, 82));
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
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

    for (auto* m : { &noiseGate, &eq4k, &comp76, &comp2a, &tube, &esser, &delay })
        content.addAndMakeVisible (*m);

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
    setupSlider (eqHfFreq, "HF Freq", "eq4k_hf_freq"); setupSlider (eqHfGain, "HF Gain", "eq4k_hf_gain"); setupSlider (eqDrive, "Drive", "eq4k_drive");
    setupBypassButton (eqLfBellButton, "LF Bell", "eq4k_lf_bell", eqLfBellAttachment);
    setupBypassButton (eqHfBellButton, "HF Bell", "eq4k_hf_bell", eqHfBellAttachment);
    setupBypassButton (eqBypassButton, "EQ IN", "eq4k_bypass", eqBypassAttachment);

    setupSlider (comp76Input, "Input", "comp76_input"); setupSlider (comp76Output, "Output", "comp76_output");
    setupSlider (comp76Attack, "Attack", "comp76_attack"); setupSlider (comp76Release, "Release", "comp76_release");
    setupChoiceButtons (comp76RatioButtons, { "4", "8", "12", "20", "All" }, "comp76_ratio");
    setupBypassButton (comp76BypassButton, "76 Bypass", "comp76_bypass", comp76BypassAttachment);
    comp76MeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (comp76MeterLabel); content.addAndMakeVisible (comp76GrMeter);

    setupSlider (comp2aPeak, "Peak Red", "comp2a_peak"); setupSlider (comp2aGain, "Gain", "comp2a_gain");
    setupSlider (comp2aEmphasis, "HF", "comp2a_emphasis"); setupSlider (comp2aMix, "Mix", "comp2a_mix");
    setupChoiceButtons (comp2aModeButtons, { "Comp", "Limit" }, "comp2a_mode");
    setupBypassButton (comp2aBypassButton, "2A Bypass", "comp2a_bypass", comp2aBypassAttachment);
    comp2aMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (comp2aMeterLabel); content.addAndMakeVisible (comp2aGrMeter);

    setupSlider (tubeBeauty, "Beauty", "tube_beauty"); setupSlider (tubeBeast, "Beast", "tube_beast");
    setupSlider (tubeSensitivity, "Sensitivity", "tube_sensitivity"); setupSlider (tubeMix, "Mix", "tube_mix");
    setupBypassButton (tubeBypassButton, "Tube Bypass", "tube_bypass", tubeBypassAttachment);

    setupSlider (esserThreshold, "Threshold", "esser_threshold"); setupSlider (esserFreq, "Freq", "esser_freq"); setupSlider (esserRange, "Range", "esser_range");
    setupChoiceButtons (esserModeButtons, { "Wide", "Split" }, "esser_mode");
    setupBypassButton (esserBypassButton, "Esser Bypass", "esser_bypass", esserBypassAttachment);
    esserMeterLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    content.addAndMakeVisible (esserMeterLabel); content.addAndMakeVisible (esserGrMeter);

    setupSlider (delayMix, "Mix", "delay_mix"); setupSlider (delayFeedback, "Feedback", "delay_feedback"); setupSlider (delayGlideTime, "Glide Time", "delay_glide_time");
    setupChoiceButtons (delayTimeButtonsA, { "1/32", "1/16", "1/8", "1/4", "1/2" }, "delay_time");
    setupChoiceButtons (delayTimeButtonsB, { "1/1", "1/8T", "1/4T", "1/8D", "1/4D" }, "delay_time");
    setupChoiceButtons (delayDirectionButtons, { "Up", "Down", "Random" }, "delay_glide_direction");
    setupBypassButton (delayGlideButton, "Glide", "delay_glide_on", delayGlideAttachment);
    setupBypassButton (delayBypassButton, "Delay Bypass", "delay_bypass", delayBypassAttachment);

    connectRackButton (noiseGate, "noisegt1_bypass"); connectRackButton (eq4k, "eq4k_bypass"); connectRackButton (comp76, "comp76_bypass");
    connectRackButton (comp2a, "comp2a_bypass"); connectRackButton (tube, "tube_bypass"); connectRackButton (esser, "esser_bypass"); connectRackButton (delay, "delay_bypass");
    installModuleDragHandlers(); commitModuleOrderToProcessor();

    syncRackVisuals(); syncChoiceButtons(); updateScaleButtonStates();
    startTimerHz (15);

    setResizable (true, true);
    setResizeLimits (800, 520, 2800, 900);
    if (auto* c = getConstrainer()) c->setMinimumOnscreenAmounts (50, 50, 50, 50);
    const int savedW = audioProcessor.getAPVTS().getRawParameterValue ("editor_width") != nullptr ? (int) audioProcessor.getAPVTS().getRawParameterValue ("editor_width")->load() : 1400;
    const int savedH = audioProcessor.getAPVTS().getRawParameterValue ("editor_height") != nullptr ? (int) audioProcessor.getAPVTS().getRawParameterValue ("editor_height")->load() : 720;
    setSize (juce::jlimit (800, 2800, savedW), juce::jlimit (520, 900, savedH));
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
    control.label.setText (labelText, juce::dontSendNotification); control.label.setJustificationType (juce::Justification::centred); control.label.setColour (juce::Label::textColourId, juce::Colours::white); content.addAndMakeVisible (control.label);
    control.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); control.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18); control.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.20f, juce::MathConstants<float>::pi * 2.80f, true); content.addAndMakeVisible (control.slider);
    control.attachment = std::make_unique<SliderAttachment> (audioProcessor.getAPVTS(), paramID, control.slider);
}

void D7SChannelStripFullAudioProcessorEditor::setupBypassButton (juce::ToggleButton& button, const juce::String& text, const juce::String& paramID, std::unique_ptr<ButtonAttachment>& attachment)
{ button.setButtonText (text); button.setColour (juce::ToggleButton::textColourId, juce::Colours::white); content.addAndMakeVisible (button); attachment = std::make_unique<ButtonAttachment> (audioProcessor.getAPVTS(), paramID, button); button.onClick = [this] { syncRackVisuals(); }; }

void D7SChannelStripFullAudioProcessorEditor::setupChoiceButtons (std::array<juce::TextButton, 5>& buttons, const juce::StringArray& labels, const juce::String& paramID)
{ for (int i = 0; i < (int) buttons.size(); ++i) { buttons[(size_t) i].setButtonText (labels[i]); content.addAndMakeVisible (buttons[(size_t) i]); buttons[(size_t) i].onClick = [this, paramID, i] { setChoiceValue (paramID, i); }; } }
void D7SChannelStripFullAudioProcessorEditor::setupChoiceButtons (std::array<juce::TextButton, 3>& buttons, const juce::StringArray& labels, const juce::String& paramID)
{ for (int i = 0; i < (int) buttons.size(); ++i) { buttons[(size_t) i].setButtonText (labels[i]); content.addAndMakeVisible (buttons[(size_t) i]); buttons[(size_t) i].onClick = [this, paramID, i] { setChoiceValue (paramID, i); }; } }
void D7SChannelStripFullAudioProcessorEditor::setupChoiceButtons (std::array<juce::TextButton, 2>& buttons, const juce::StringArray& labels, const juce::String& paramID)
{ for (int i = 0; i < (int) buttons.size(); ++i) { buttons[(size_t) i].setButtonText (labels[i]); content.addAndMakeVisible (buttons[(size_t) i]); buttons[(size_t) i].onClick = [this, paramID, i] { setChoiceValue (paramID, i); }; } }

void D7SChannelStripFullAudioProcessorEditor::setChoiceValue (const juce::String& paramID, int index)
{ if (auto* p = audioProcessor.getAPVTS().getParameter (paramID)) { const float normalised = p->convertTo0to1 ((float) index); p->beginChangeGesture(); p->setValueNotifyingHost (normalised); p->endChangeGesture(); } syncChoiceButtons(); }

void D7SChannelStripFullAudioProcessorEditor::syncChoiceButtons()
{
    auto sync = [this] (auto& buttons, const char* id, int offset = 0) { int active = 0; if (auto* v = audioProcessor.getAPVTS().getRawParameterValue (id)) active = (int) std::round (v->load()); for (int i = 0; i < (int) buttons.size(); ++i) { buttons[(size_t) i].setClickingTogglesState (false); const bool on = ((i + offset) == active); buttons[(size_t) i].setColour (juce::TextButton::buttonColourId, on ? juce::Colour (90, 90, 90) : juce::Colour (32, 32, 32)); buttons[(size_t) i].setColour (juce::TextButton::textColourOffId, juce::Colours::white); } };
    sync (comp76RatioButtons, "comp76_ratio"); sync (comp2aModeButtons, "comp2a_mode"); sync (esserModeButtons, "esser_mode"); sync (delayTimeButtonsA, "delay_time", 0); sync (delayTimeButtonsB, "delay_time", 5); sync (delayDirectionButtons, "delay_glide_direction");
}

void D7SChannelStripFullAudioProcessorEditor::connectRackButton (RackModuleComponent& module, const juce::String& bypassParamID)
{ module.onEnabledChanged = [this, bypassParamID] (bool enabled) { if (auto* param = audioProcessor.getAPVTS().getParameter (bypassParamID)) { param->beginChangeGesture(); param->setValueNotifyingHost (enabled ? 0.0f : 1.0f); param->endChangeGesture(); } syncRackVisuals(); }; }

void D7SChannelStripFullAudioProcessorEditor::syncRackVisuals()
{ auto& apvts = audioProcessor.getAPVTS(); auto syncOne = [&apvts] (RackModuleComponent& module, const char* paramID) { if (auto* value = apvts.getRawParameterValue (paramID)) module.setEnabledState (value->load() <= 0.5f); }; syncOne (noiseGate, "noisegt1_bypass"); syncOne (eq4k, "eq4k_bypass"); syncOne (comp76, "comp76_bypass"); syncOne (comp2a, "comp2a_bypass"); syncOne (tube, "tube_bypass"); syncOne (esser, "esser_bypass"); syncOne (delay, "delay_bypass"); }

void D7SChannelStripFullAudioProcessorEditor::installModuleDragHandlers()
{ auto install = [this] (RackModuleComponent& module) { module.onDragStart = [this] (RackModuleComponent* m, const juce::MouseEvent&) { draggingModule = m; if (m != nullptr) m->toFront (false); }; module.onDragMove = [this] (RackModuleComponent* m, const juce::MouseEvent& e) { if (m == nullptr) return; const int moduleId = getModuleIdForComponent (m); if (moduleId < 0) return; auto contentEvent = e.getEventRelativeTo (&content); const int targetSlot = getSlotForMousePosition (contentEvent.getPosition()); if (targetSlot >= 0) moveModuleToSlot (moduleId, targetSlot); }; module.onDragEnd = [this] (RackModuleComponent*, const juce::MouseEvent&) { draggingModule = nullptr; commitModuleOrderToProcessor(); resized(); }; }; install (noiseGate); install (eq4k); install (comp76); install (comp2a); install (tube); install (esser); install (delay); }

int D7SChannelStripFullAudioProcessorEditor::getModuleIdForComponent (RackModuleComponent* module) const noexcept
{ if (module == &noiseGate) return D7SChannelStripFullAudioProcessor::moduleNoiseGT1; if (module == &eq4k) return D7SChannelStripFullAudioProcessor::moduleEQ4K; if (module == &comp76) return D7SChannelStripFullAudioProcessor::module76; if (module == &comp2a) return D7SChannelStripFullAudioProcessor::module2A; if (module == &tube) return D7SChannelStripFullAudioProcessor::moduleTube; if (module == &esser) return D7SChannelStripFullAudioProcessor::moduleEsser; if (module == &delay) return D7SChannelStripFullAudioProcessor::moduleDelay; return -1; }

RackModuleComponent* D7SChannelStripFullAudioProcessorEditor::getModuleHeaderForId (int moduleId) noexcept
{ switch (moduleId) { case D7SChannelStripFullAudioProcessor::moduleNoiseGT1: return &noiseGate; case D7SChannelStripFullAudioProcessor::moduleEQ4K: return &eq4k; case D7SChannelStripFullAudioProcessor::module76: return &comp76; case D7SChannelStripFullAudioProcessor::module2A: return &comp2a; case D7SChannelStripFullAudioProcessor::moduleTube: return &tube; case D7SChannelStripFullAudioProcessor::moduleEsser: return &esser; case D7SChannelStripFullAudioProcessor::moduleDelay: return &delay; default: return nullptr; } }

int D7SChannelStripFullAudioProcessorEditor::getSlotForMousePosition (juce::Point<int> contentPoint) const noexcept
{ for (int i = 0; i < D7SChannelStripFullAudioProcessor::numRackModules; ++i) if (moduleSlotBounds[(size_t) i].expanded (8, 120).contains (contentPoint)) return i; for (int i = 0; i < D7SChannelStripFullAudioProcessor::numRackModules; ++i) if (contentPoint.x < moduleSlotBounds[(size_t) i].getCentreX()) return i; return D7SChannelStripFullAudioProcessor::numRackModules - 1; }

void D7SChannelStripFullAudioProcessorEditor::moveModuleToSlot (int moduleId, int targetSlot)
{ targetSlot = juce::jlimit (0, D7SChannelStripFullAudioProcessor::numRackModules - 1, targetSlot); int currentSlot = -1; for (int i = 0; i < D7SChannelStripFullAudioProcessor::numRackModules; ++i) if (editorModuleOrder[(size_t) i] == moduleId) { currentSlot = i; break; } if (currentSlot < 0 || currentSlot == targetSlot) return; if (currentSlot < targetSlot) for (int i = currentSlot; i < targetSlot; ++i) editorModuleOrder[(size_t) i] = editorModuleOrder[(size_t) i + 1]; else for (int i = currentSlot; i > targetSlot; --i) editorModuleOrder[(size_t) i] = editorModuleOrder[(size_t) i - 1]; editorModuleOrder[(size_t) targetSlot] = moduleId; commitModuleOrderToProcessor(); resized(); }

void D7SChannelStripFullAudioProcessorEditor::commitModuleOrderToProcessor() { audioProcessor.setModuleOrder (editorModuleOrder); }

void D7SChannelStripFullAudioProcessorEditor::timerCallback()
{ const auto inDb = audioProcessor.getRackInputPeakDb(); const auto outDb = audioProcessor.getRackOutputPeakDb(); rackMeterLabel.setText ("Rack In " + dbText (inDb) + " / Out " + dbText (outDb), juce::dontSendNotification); rackInMeter.setDbValue (inDb, false); rackOutMeter.setDbValue (outDb, false); const auto nGr = audioProcessor.getNoiseGT1GainReductionDb(); noiseMeterLabel.setText ("GR " + juce::String (nGr, 1) + " dB", juce::dontSendNotification); noiseGrMeter.setDbValue (nGr, true); const auto c76 = audioProcessor.getComp76GainReductionDb(); comp76MeterLabel.setText ("GR " + juce::String (c76, 1) + " dB", juce::dontSendNotification); comp76GrMeter.setDbValue (c76, true); const auto c2a = audioProcessor.getComp2AGainReductionDb(); comp2aMeterLabel.setText ("GR " + juce::String (c2a, 1) + " dB", juce::dontSendNotification); comp2aGrMeter.setDbValue (c2a, true); const auto es = audioProcessor.getEsserGainReductionDb(); esserMeterLabel.setText ("GR " + juce::String (es, 1) + " dB", juce::dontSendNotification); esserGrMeter.setDbValue (es, true); spectrumView.repaint(); syncChoiceButtons(); }

void D7SChannelStripFullAudioProcessorEditor::layoutModuleControls (juce::Rectangle<int>& area, RackModuleComponent& module, std::initializer_list<ParamSlider*> sliders, juce::ToggleButton& bypassButton)
{ module.setBounds (area.removeFromTop (56)); area.removeFromTop (4); auto controlsArea = area.removeFromTop (90); for (auto* control : sliders) { auto group = controlsArea.removeFromLeft (78); control->label.setBounds (group.removeFromTop (18)); control->slider.setBounds (group); controlsArea.removeFromLeft (8); } bypassButton.setBounds (controlsArea.removeFromLeft (150).withHeight (28)); area.removeFromTop (6); }

void D7SChannelStripFullAudioProcessorEditor::paint (juce::Graphics& g)
{ g.fillAll (juce::Colour (20, 20, 20)); g.setColour (juce::Colours::white); g.setFont (20.0f); g.drawText ("D7S CHANNEL STRIP", 18, 0, 360, 36, juce::Justification::centredLeft); g.setFont (13.0f); g.drawText ("Scale", getWidth() - 260, 0, 54, 36, juce::Justification::centredRight); }

void D7SChannelStripFullAudioProcessorEditor::resized()
{
    setHostParamValue (audioProcessor.getAPVTS(), "editor_width", (float) getWidth());
    setHostParamValue (audioProcessor.getAPVTS(), "editor_height", (float) getHeight());

    auto top = getLocalBounds().removeFromTop (36).reduced (8, 5);
    auto scaleArea = top.removeFromRight (200);
    scale100Button.setBounds (scaleArea.removeFromLeft (48)); scale75Button.setBounds (scaleArea.removeFromLeft (48)); scale50Button.setBounds (scaleArea.removeFromLeft (48)); scale25Button.setBounds (scaleArea.removeFromLeft (48));

    content.setBounds (0, 38, designWidth, designHeight); content.setTransform (juce::AffineTransform::scale (uiScale));
    auto area = juce::Rectangle<int> (0, 0, designWidth, designHeight).reduced (18);
    auto layoutKnobLocal = [] (auto& control, juce::Rectangle<int> r) { control.label.setBounds (r.removeFromTop (18)); control.slider.setBounds (r); };
    auto panelHeader = [] (juce::Rectangle<int>& panel, RackModuleComponent& module) { module.setBounds (panel.removeFromTop (56)); panel.removeFromTop (8); };

    auto header = area.removeFromTop (120);
    auto rackControls = header.removeFromLeft (315);
    for (auto* control : { &rackInput, &rackOutput, &rackMix }) { auto group = rackControls.removeFromLeft (92); layoutKnobLocal (*control, group.removeFromTop (90)); rackControls.removeFromLeft (8); }
    auto meterArea = header.removeFromLeft (390); rackMeterLabel.setBounds (meterArea.removeFromTop (24)); rackInMeter.setBounds (meterArea.removeFromTop (18)); meterArea.removeFromTop (8); rackOutMeter.setBounds (meterArea.removeFromTop (18)); spectrumView.setBounds (header.reduced (8, 0)); area.removeFromTop (10);

    auto getPanelWidth = [] (int moduleId) { switch (moduleId) { case D7SChannelStripFullAudioProcessor::moduleNoiseGT1: return 180; case D7SChannelStripFullAudioProcessor::moduleEQ4K: return 390; case D7SChannelStripFullAudioProcessor::module76: return 230; case D7SChannelStripFullAudioProcessor::module2A: return 260; case D7SChannelStripFullAudioProcessor::moduleTube: return 250; case D7SChannelStripFullAudioProcessor::moduleEsser: return 260; case D7SChannelStripFullAudioProcessor::moduleDelay: return 320; default: return 220; } };
    auto layoutModuleById = [&] (int moduleId, juce::Rectangle<int> panel)
    {
        switch (moduleId)
        {
            case D7SChannelStripFullAudioProcessor::moduleNoiseGT1: { panelHeader (panel, noiseGate); layoutKnobLocal (noiseSuppression, panel.removeFromTop (96).withWidth (86)); noiseBypassButton.setBounds (panel.removeFromTop (28)); panel.removeFromTop (8); noiseMeterLabel.setBounds (panel.removeFromTop (20)); noiseGrMeter.setBounds (panel.removeFromTop (16)); break; }
            case D7SChannelStripFullAudioProcessor::moduleEQ4K: { panelHeader (panel, eq4k); auto filterRow = panel.removeFromTop (105); layoutKnobLocal (eqHpf, nextRowLeft (filterRow)); layoutKnobLocal (eqLpf, nextRowLeft (filterRow)); layoutKnobLocal (eqDrive, nextRowLeft (filterRow)); eqBypassButton.setBounds (filterRow.removeFromLeft (90).withHeight (28)); panel.removeFromTop (8); auto hfRow = panel.removeFromTop (105); layoutKnobLocal (eqHfGain, nextRowLeft (hfRow)); layoutKnobLocal (eqHfFreq, nextRowLeft (hfRow)); eqHfBellButton.setBounds (hfRow.removeFromLeft (90).withHeight (28)); panel.removeFromTop (8); auto hmfRow = panel.removeFromTop (105); layoutKnobLocal (eqHmfGain, nextRowLeft (hmfRow)); layoutKnobLocal (eqHmfFreq, nextRowLeft (hmfRow)); layoutKnobLocal (eqHmfQ, nextRowLeft (hmfRow)); panel.removeFromTop (8); auto lmfRow = panel.removeFromTop (105); layoutKnobLocal (eqLmfGain, nextRowLeft (lmfRow)); layoutKnobLocal (eqLmfFreq, nextRowLeft (lmfRow)); layoutKnobLocal (eqLmfQ, nextRowLeft (lmfRow)); panel.removeFromTop (8); auto lfRow = panel.removeFromTop (105); layoutKnobLocal (eqLfGain, nextRowLeft (lfRow)); layoutKnobLocal (eqLfFreq, nextRowLeft (lfRow)); eqLfBellButton.setBounds (lfRow.removeFromLeft (90).withHeight (28)); break; }
            case D7SChannelStripFullAudioProcessor::module76: { panelHeader (panel, comp76); auto c76Top = panel.removeFromTop (105); layoutKnobLocal (comp76Input, nextRowLeft (c76Top)); layoutKnobLocal (comp76Output, nextRowLeft (c76Top)); auto c76Mid = panel.removeFromTop (105); layoutKnobLocal (comp76Attack, nextRowLeft (c76Mid)); layoutKnobLocal (comp76Release, nextRowLeft (c76Mid)); comp76BypassButton.setBounds (panel.removeFromTop (28)); panel.removeFromTop (8); auto ratioArea = panel.removeFromTop (30); for (auto& b : comp76RatioButtons) { b.setBounds (ratioArea.removeFromLeft (40)); ratioArea.removeFromLeft (4); } panel.removeFromTop (8); comp76MeterLabel.setBounds (panel.removeFromTop (20)); comp76GrMeter.setBounds (panel.removeFromTop (16)); break; }
            case D7SChannelStripFullAudioProcessor::module2A: { panelHeader (panel, comp2a); auto c2aTop = panel.removeFromTop (105); layoutKnobLocal (comp2aPeak, nextRowLeft (c2aTop)); layoutKnobLocal (comp2aGain, nextRowLeft (c2aTop)); auto c2aMid = panel.removeFromTop (105); layoutKnobLocal (comp2aEmphasis, nextRowLeft (c2aMid)); layoutKnobLocal (comp2aMix, nextRowLeft (c2aMid)); comp2aBypassButton.setBounds (panel.removeFromTop (28)); panel.removeFromTop (8); auto mode2a = panel.removeFromTop (30); for (auto& b : comp2aModeButtons) { b.setBounds (mode2a.removeFromLeft (78)); mode2a.removeFromLeft (6); } panel.removeFromTop (8); comp2aMeterLabel.setBounds (panel.removeFromTop (20)); comp2aGrMeter.setBounds (panel.removeFromTop (16)); break; }
            case D7SChannelStripFullAudioProcessor::moduleTube: { panelHeader (panel, tube); auto tubeTop = panel.removeFromTop (105); layoutKnobLocal (tubeBeauty, nextRowLeft (tubeTop)); layoutKnobLocal (tubeBeast, nextRowLeft (tubeTop)); auto tubeMid = panel.removeFromTop (105); layoutKnobLocal (tubeSensitivity, nextRowLeft (tubeMid)); layoutKnobLocal (tubeMix, nextRowLeft (tubeMid)); tubeBypassButton.setBounds (panel.removeFromTop (28)); break; }
            case D7SChannelStripFullAudioProcessor::moduleEsser: { panelHeader (panel, esser); auto esTop = panel.removeFromTop (105); layoutKnobLocal (esserThreshold, nextRowLeft (esTop)); layoutKnobLocal (esserFreq, nextRowLeft (esTop)); auto esMid = panel.removeFromTop (105); layoutKnobLocal (esserRange, nextRowLeft (esMid)); esserBypassButton.setBounds (esMid.removeFromLeft (120).withHeight (28)); auto modeEsser = panel.removeFromTop (30); for (auto& b : esserModeButtons) { b.setBounds (modeEsser.removeFromLeft (82)); modeEsser.removeFromLeft (6); } panel.removeFromTop (8); esserMeterLabel.setBounds (panel.removeFromTop (20)); esserGrMeter.setBounds (panel.removeFromTop (16)); break; }
            case D7SChannelStripFullAudioProcessor::moduleDelay: { panelHeader (panel, delay); auto dTop = panel.removeFromTop (105); layoutKnobLocal (delayMix, nextRowLeft (dTop)); layoutKnobLocal (delayFeedback, nextRowLeft (dTop)); layoutKnobLocal (delayGlideTime, nextRowLeft (dTop)); delayBypassButton.setBounds (panel.removeFromTop (28)); panel.removeFromTop (6); auto timeA = panel.removeFromTop (28); for (auto& b : delayTimeButtonsA) { b.setBounds (timeA.removeFromLeft (52)); timeA.removeFromLeft (4); } auto timeB = panel.removeFromTop (28); for (auto& b : delayTimeButtonsB) { b.setBounds (timeB.removeFromLeft (52)); timeB.removeFromLeft (4); } panel.removeFromTop (6); auto glideRow = panel.removeFromTop (28); delayGlideButton.setBounds (glideRow.removeFromLeft (72)); glideRow.removeFromLeft (8); for (auto& b : delayDirectionButtons) { b.setBounds (glideRow.removeFromLeft (70)); glideRow.removeFromLeft (4); } break; }
            default: break;
        }
    };

    auto modules = area; const int gap = 10;
    for (int slot = 0; slot < D7SChannelStripFullAudioProcessor::numRackModules; ++slot)
    { const int moduleId = editorModuleOrder[(size_t) slot]; const int width = getPanelWidth (moduleId); auto panel = modules.removeFromLeft (width); moduleSlotBounds[(size_t) slot] = panel; layoutModuleById (moduleId, panel); modules.removeFromLeft (gap); }
}
