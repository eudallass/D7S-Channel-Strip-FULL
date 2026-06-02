#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/RackModuleComponent.h"

class D7SChannelStripFullAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit D7SChannelStripFullAudioProcessorEditor (D7SChannelStripFullAudioProcessor&);
    ~D7SChannelStripFullAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct ParamSlider
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct ParamChoice
    {
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<ComboBoxAttachment> attachment;
    };

    void setUIScale (float newScale);
    void updateScaleButtonStates();

    void setupSlider (ParamSlider& control, const juce::String& labelText, const juce::String& paramID);
    void setupChoice (ParamChoice& control, const juce::String& labelText, const juce::String& paramID, const juce::StringArray& items);
    void setupBypassButton (juce::ToggleButton& button, const juce::String& text, const juce::String& paramID, std::unique_ptr<ButtonAttachment>& attachment);
    void connectRackButton (RackModuleComponent& module, const juce::String& bypassParamID);
    void syncRackVisuals();
    void timerCallback() override;

    void layoutModuleControls (juce::Rectangle<int>& area,
                               RackModuleComponent& module,
                               std::initializer_list<ParamSlider*> sliders,
                               std::initializer_list<ParamChoice*> choices,
                               juce::ToggleButton& bypassButton);

    D7SChannelStripFullAudioProcessor& audioProcessor;

    float uiScale { 0.75f };
    juce::Component content;
    juce::TextButton scale100Button { "100%" };
    juce::TextButton scale75Button  { "75%" };
    juce::TextButton scale50Button  { "50%" };
    juce::TextButton scale25Button  { "25%" };

    RackModuleComponent noiseGate { "D7S NoiseGT1" };
    RackModuleComponent eq4k      { "D7S EQ 4K" };
    RackModuleComponent comp76    { "D7S 76" };
    RackModuleComponent comp2a    { "D7S 2A" };
    RackModuleComponent tube      { "D7S Tube" };
    RackModuleComponent esser     { "D7S Esser" };

    ParamSlider rackInput;
    ParamSlider rackOutput;
    ParamSlider rackMix;
    juce::Label rackMeterLabel;

    ParamSlider noiseSuppression;
    juce::Label noiseMeterLabel;
    juce::ToggleButton noiseBypassButton;
    std::unique_ptr<ButtonAttachment> noiseBypassAttachment;

    ParamSlider eqHpf;
    ParamSlider eqLpf;
    ParamSlider eqLfFreq;
    ParamSlider eqLfGain;
    ParamSlider eqLmfFreq;
    ParamSlider eqLmfGain;
    ParamSlider eqLmfQ;
    ParamSlider eqHmfFreq;
    ParamSlider eqHmfGain;
    ParamSlider eqHmfQ;
    ParamSlider eqHfFreq;
    ParamSlider eqHfGain;
    juce::ToggleButton eqLfBellButton;
    juce::ToggleButton eqHfBellButton;
    juce::ToggleButton eqBypassButton;
    std::unique_ptr<ButtonAttachment> eqLfBellAttachment;
    std::unique_ptr<ButtonAttachment> eqHfBellAttachment;
    std::unique_ptr<ButtonAttachment> eqBypassAttachment;

    ParamSlider comp76Input;
    ParamSlider comp76Output;
    ParamSlider comp76Attack;
    ParamSlider comp76Release;
    ParamChoice comp76Ratio;
    juce::Label comp76MeterLabel;
    juce::ToggleButton comp76BypassButton;
    std::unique_ptr<ButtonAttachment> comp76BypassAttachment;

    ParamSlider comp2aPeak;
    ParamSlider comp2aGain;
    ParamSlider comp2aEmphasis;
    ParamSlider comp2aMix;
    ParamChoice comp2aMode;
    juce::Label comp2aMeterLabel;
    juce::ToggleButton comp2aBypassButton;
    std::unique_ptr<ButtonAttachment> comp2aBypassAttachment;

    ParamSlider tubeBeauty;
    ParamSlider tubeBeast;
    ParamSlider tubeSensitivity;
    juce::ToggleButton tubeBypassButton;
    std::unique_ptr<ButtonAttachment> tubeBypassAttachment;

    ParamSlider esserThreshold;
    ParamSlider esserFreq;
    ParamSlider esserRange;
    ParamChoice esserMode;
    juce::Label esserMeterLabel;
    juce::ToggleButton esserBypassButton;
    std::unique_ptr<ButtonAttachment> esserBypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessorEditor)
};
