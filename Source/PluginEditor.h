#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/RackModuleComponent.h"

class D7SChannelStripFullAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit D7SChannelStripFullAudioProcessorEditor (D7SChannelStripFullAudioProcessor&);
    ~D7SChannelStripFullAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct ParamSlider
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setupSlider (ParamSlider& control, const juce::String& labelText, const juce::String& paramID);
    void setupBypassButton (juce::ToggleButton& button, const juce::String& text, const juce::String& paramID, std::unique_ptr<ButtonAttachment>& attachment);
    void connectRackButton (RackModuleComponent& module, const juce::String& bypassParamID);
    void syncRackVisuals();
    void layoutModuleControls (juce::Rectangle<int>& area, RackModuleComponent& module, std::initializer_list<ParamSlider*> sliders, juce::ToggleButton& bypassButton);

    D7SChannelStripFullAudioProcessor& audioProcessor;

    RackModuleComponent noiseGate { "D7S NoiseGT1" };
    RackModuleComponent eq4k      { "D7S EQ 4K" };
    RackModuleComponent comp76    { "D7S 76" };
    RackModuleComponent comp2a    { "D7S 2A" };
    RackModuleComponent tube      { "D7S Tube" };
    RackModuleComponent esser     { "D7S Esser" };

    ParamSlider noiseSuppression;
    juce::ToggleButton noiseBypassButton;
    std::unique_ptr<ButtonAttachment> noiseBypassAttachment;

    ParamSlider eqLow;
    ParamSlider eqLowMid;
    ParamSlider eqPresence;
    ParamSlider eqAir;
    juce::ToggleButton eqBypassButton;
    std::unique_ptr<ButtonAttachment> eqBypassAttachment;

    ParamSlider comp76Input;
    ParamSlider comp76Attack;
    ParamSlider comp76Release;
    ParamSlider comp76Output;
    juce::ToggleButton comp76BypassButton;
    std::unique_ptr<ButtonAttachment> comp76BypassAttachment;

    ParamSlider comp2aPeak;
    ParamSlider comp2aGain;
    juce::ToggleButton comp2aBypassButton;
    std::unique_ptr<ButtonAttachment> comp2aBypassAttachment;

    ParamSlider tubeDrive;
    ParamSlider tubeTone;
    juce::ToggleButton tubeBypassButton;
    std::unique_ptr<ButtonAttachment> tubeBypassAttachment;

    ParamSlider esserFreq;
    ParamSlider esserAmount;
    juce::ToggleButton esserBypassButton;
    std::unique_ptr<ButtonAttachment> esserBypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessorEditor)
};
