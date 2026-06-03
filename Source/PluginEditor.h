#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "PluginProcessor.h"
#include "UI/RackModuleComponent.h"
#include "UI/SpectrumDisplay.h"

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

    struct ParamSlider
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<SliderAttachment> attachment;
    };

    class HorizontalMeter : public juce::Component
    {
    public:
        void setDbValue (float newDb, bool isGainReduction);
        void paint (juce::Graphics& g) override;
    private:
        float dbValue { -120.0f };
        bool grMode { false };
    };

    // Kept as a temporary compatibility shim while SpectrumDisplay replaces the old analyzer.
    class SpectrumView : public juce::Component
    {
    public:
        explicit SpectrumView (D7SChannelStripFullAudioProcessor& p) : processor (p) {}
        void paint (juce::Graphics& g) override;
    private:
        D7SChannelStripFullAudioProcessor& processor;
    };

    void setUIScale (float newScale);
    void updateScaleButtonStates();

    void setupSlider (ParamSlider& control, const juce::String& labelText, const juce::String& paramID);
    void setupBypassButton (juce::ToggleButton& button, const juce::String& text, const juce::String& paramID, std::unique_ptr<ButtonAttachment>& attachment);
    void setupChoiceButtons (std::array<juce::TextButton, 5>& buttons, const juce::StringArray& labels, const juce::String& paramID);
    void setupChoiceButtons (std::array<juce::TextButton, 3>& buttons, const juce::StringArray& labels, const juce::String& paramID);
    void setupChoiceButtons (std::array<juce::TextButton, 2>& buttons, const juce::StringArray& labels, const juce::String& paramID);
    void setChoiceValue (const juce::String& paramID, int index);
    void syncChoiceButtons();
    void connectRackButton (RackModuleComponent& module, const juce::String& bypassParamID);
    void syncRackVisuals();
    void timerCallback() override;

    void installModuleDragHandlers();
    int getModuleIdForComponent (RackModuleComponent* module) const noexcept;
    RackModuleComponent* getModuleHeaderForId (int moduleId) noexcept;
    int getSlotForMousePosition (juce::Point<int> contentPoint) const noexcept;
    void moveModuleToSlot (int moduleId, int targetSlot);
    void commitModuleOrderToProcessor();

    void layoutModuleControls (juce::Rectangle<int>& area,
                               RackModuleComponent& module,
                               std::initializer_list<ParamSlider*> sliders,
                               juce::ToggleButton& bypassButton);

    D7SChannelStripFullAudioProcessor& audioProcessor;

    float uiScale { 0.75f };
    juce::Component content;
    juce::TextButton scale100Button { "100%" };
    juce::TextButton scale75Button  { "75%" };
    juce::TextButton scale50Button  { "50%" };
    juce::TextButton scale25Button  { "25%" };

    std::array<int, D7SChannelStripFullAudioProcessor::numRackModules> editorModuleOrder {
        D7SChannelStripFullAudioProcessor::moduleNoiseGT1,
        D7SChannelStripFullAudioProcessor::moduleEQ4K,
        D7SChannelStripFullAudioProcessor::module76,
        D7SChannelStripFullAudioProcessor::module2A,
        D7SChannelStripFullAudioProcessor::moduleTube,
        D7SChannelStripFullAudioProcessor::moduleEsser,
        D7SChannelStripFullAudioProcessor::moduleDelay
    };
    std::array<juce::Rectangle<int>, D7SChannelStripFullAudioProcessor::numRackModules> moduleSlotBounds {};
    RackModuleComponent* draggingModule { nullptr };

    RackModuleComponent noiseGate { "D7S NoiseGT1" };
    RackModuleComponent eq4k      { "D7S EQ 4K" };
    RackModuleComponent comp76    { "D7S 76" };
    RackModuleComponent comp2a    { "D7S 2A" };
    RackModuleComponent tube      { "D7S Tube" };
    RackModuleComponent esser     { "D7S Esser" };
    RackModuleComponent delay     { "D7S Delay" };

    ParamSlider rackInput;
    ParamSlider rackOutput;
    ParamSlider rackMix;
    juce::Label rackMeterLabel;
    HorizontalMeter rackInMeter;
    HorizontalMeter rackOutMeter;
    SpectrumDisplay spectrumView { audioProcessor };

    ParamSlider noiseSuppression;
    juce::Label noiseMeterLabel;
    HorizontalMeter noiseGrMeter;
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
    std::array<juce::TextButton, 5> comp76RatioButtons { juce::TextButton { "4" }, juce::TextButton { "8" }, juce::TextButton { "12" }, juce::TextButton { "20" }, juce::TextButton { "All" } };
    juce::Label comp76MeterLabel;
    HorizontalMeter comp76GrMeter;
    juce::ToggleButton comp76BypassButton;
    std::unique_ptr<ButtonAttachment> comp76BypassAttachment;

    ParamSlider comp2aPeak;
    ParamSlider comp2aGain;
    ParamSlider comp2aEmphasis;
    ParamSlider comp2aMix;
    std::array<juce::TextButton, 2> comp2aModeButtons { juce::TextButton { "Comp" }, juce::TextButton { "Limit" } };
    juce::Label comp2aMeterLabel;
    HorizontalMeter comp2aGrMeter;
    juce::ToggleButton comp2aBypassButton;
    std::unique_ptr<ButtonAttachment> comp2aBypassAttachment;

    ParamSlider tubeBeauty;
    ParamSlider tubeBeast;
    ParamSlider tubeSensitivity;
    ParamSlider tubeMix;
    juce::ToggleButton tubeBypassButton;
    std::unique_ptr<ButtonAttachment> tubeBypassAttachment;

    ParamSlider esserThreshold;
    ParamSlider esserFreq;
    ParamSlider esserRange;
    std::array<juce::TextButton, 2> esserModeButtons { juce::TextButton { "Wide" }, juce::TextButton { "Split" } };
    juce::Label esserMeterLabel;
    HorizontalMeter esserGrMeter;
    juce::ToggleButton esserBypassButton;
    std::unique_ptr<ButtonAttachment> esserBypassAttachment;

    ParamSlider delayMix;
    ParamSlider delayFeedback;
    ParamSlider delayGlideTime;
    std::array<juce::TextButton, 5> delayTimeButtonsA { juce::TextButton { "1/32" }, juce::TextButton { "1/16" }, juce::TextButton { "1/8" }, juce::TextButton { "1/4" }, juce::TextButton { "1/2" } };
    std::array<juce::TextButton, 5> delayTimeButtonsB { juce::TextButton { "1/1" }, juce::TextButton { "1/8T" }, juce::TextButton { "1/4T" }, juce::TextButton { "1/8D" }, juce::TextButton { "1/4D" } };
    std::array<juce::TextButton, 3> delayDirectionButtons { juce::TextButton { "Up" }, juce::TextButton { "Down" }, juce::TextButton { "Random" } };
    juce::ToggleButton delayGlideButton;
    juce::ToggleButton delayBypassButton;
    std::unique_ptr<ButtonAttachment> delayGlideAttachment;
    std::unique_ptr<ButtonAttachment> delayBypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessorEditor)
};
