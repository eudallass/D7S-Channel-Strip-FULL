#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "PluginProcessor.h"
#include "UI/RackModuleComponent.h"
#include "UI/SpectrumAnalyzerComponent.h"

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

    class D7SScrollBarLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawScrollbar (juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override
        {
            juce::ignoreUnused (scrollbar);
            auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
            g.setColour (juce::Colour (12, 14, 18).withAlpha (0.92f));
            g.fillRoundedRectangle (bounds.reduced (1.0f), 5.0f);
            juce::Rectangle<float> thumb;
            if (isScrollbarVertical) thumb = { (float) x + 2.0f, (float) thumbStartPosition + 2.0f, (float) width - 4.0f, (float) thumbSize - 4.0f };
            else thumb = { (float) thumbStartPosition + 2.0f, (float) y + 2.0f, (float) thumbSize - 4.0f, (float) height - 4.0f };
            const auto base = isMouseDown ? juce::Colour (255, 200, 88) : isMouseOver ? juce::Colour (255, 180, 60) : juce::Colour (150, 126, 82);
            g.setColour (base.withAlpha (isMouseDown ? 0.95f : 0.72f));
            g.fillRoundedRectangle (thumb, 4.0f);
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawRoundedRectangle (thumb, 4.0f, 1.0f);
        }
    };

    class RackViewport final : public juce::Viewport
    {
    public:
        RackViewport() { setLookAndFeel (&scrollbarLookAndFeel); setScrollBarsShown (false, true); setScrollBarThickness (12); setSingleStepSizes (40, 40); }
        ~RackViewport() override { setLookAndFeel (nullptr); }
        void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override { if (e.mods.isShiftDown() && wheel.deltaY != 0.0f) { auto horizontal = wheel; horizontal.deltaX = wheel.deltaY; horizontal.deltaY = 0.0f; juce::Viewport::mouseWheelMove (e, horizontal); repaint(); return; } juce::Viewport::mouseWheelMove (e, wheel); repaint(); }
        void visibleAreaChanged (const juce::Rectangle<int>&) override { repaint(); }
        void paintOverChildren (juce::Graphics& g) override { auto* viewed = getViewedComponent(); if (viewed == nullptr) return; const auto pos = getViewPosition(); const bool canLeft = pos.x > 0; const bool canRight = pos.x + getMaximumVisibleWidth() < viewed->getWidth(); auto drawArrow = [&] (bool left) { const float x = left ? 8.0f : (float) getWidth() - 8.0f; const float y = (float) getHeight() * 0.50f; juce::Path p; if (left) p.addTriangle (x + 5.0f, y - 10.0f, x - 5.0f, y, x + 5.0f, y + 10.0f); else p.addTriangle (x - 5.0f, y - 10.0f, x + 5.0f, y, x - 5.0f, y + 10.0f); g.setColour (juce::Colour (255, 180, 60).withAlpha (0.42f)); g.fillPath (p); }; if (canLeft) drawArrow (true); if (canRight) drawArrow (false); }
    private:
        D7SScrollBarLookAndFeel scrollbarLookAndFeel;
    };

    struct ParamSlider { juce::Label label; juce::Slider slider; std::unique_ptr<SliderAttachment> attachment; };
    class HorizontalMeter : public juce::Component { public: void setDbValue (float newDb, bool isGainReduction); void paint (juce::Graphics& g) override; private: float dbValue { -120.0f }; bool grMode { false }; };
    

    void setUIScale (float newScale); void updateScaleButtonStates(); void setupSlider (ParamSlider& control, const juce::String& labelText, const juce::String& paramID); void setupBypassButton (juce::ToggleButton& button, const juce::String& text, const juce::String& paramID, std::unique_ptr<ButtonAttachment>& attachment); void setupChoiceButtons (std::array<juce::TextButton, 5>& buttons, const juce::StringArray& labels, const juce::String& paramID); void setupChoiceButtons (std::array<juce::TextButton, 3>& buttons, const juce::StringArray& labels, const juce::String& paramID); void setupChoiceButtons (std::array<juce::TextButton, 2>& buttons, const juce::StringArray& labels, const juce::String& paramID); void setChoiceValue (const juce::String& paramID, int index); void syncChoiceButtons(); void syncDelayTimeLabel(); juce::String formatDelayFraction() const; void connectRackButton (RackModuleComponent& module, const juce::String& bypassParamID); void syncRackVisuals(); void timerCallback() override; void installModuleDragHandlers(); int getModuleIdForComponent (RackModuleComponent* module) const noexcept; RackModuleComponent* getModuleHeaderForId (int moduleId) noexcept; int getSlotForMousePosition (juce::Point<int> contentPoint) const noexcept; void moveModuleToSlot (int moduleId, int targetSlot); void commitModuleOrderToProcessor(); void layoutModuleControls (juce::Rectangle<int>& area, RackModuleComponent& module, std::initializer_list<ParamSlider*> sliders, juce::ToggleButton& bypassButton);

    D7SChannelStripFullAudioProcessor& audioProcessor; float uiScale { 0.75f }; RackViewport rackViewport; juce::Component content; juce::TextButton scale100Button { "100%" }; juce::TextButton scale75Button { "75%" }; juce::TextButton scale50Button { "50%" }; juce::TextButton scale25Button { "25%" };
    std::array<int, D7SChannelStripFullAudioProcessor::numRackModules> editorModuleOrder { D7SChannelStripFullAudioProcessor::moduleNoiseGT1, D7SChannelStripFullAudioProcessor::moduleEQ4K, D7SChannelStripFullAudioProcessor::module76, D7SChannelStripFullAudioProcessor::module2A, D7SChannelStripFullAudioProcessor::moduleTube, D7SChannelStripFullAudioProcessor::moduleClipper, D7SChannelStripFullAudioProcessor::moduleEsser, D7SChannelStripFullAudioProcessor::moduleDelay };
    std::array<juce::Rectangle<int>, D7SChannelStripFullAudioProcessor::numRackModules> moduleSlotBounds {}; RackModuleComponent* draggingModule { nullptr };
    RackModuleComponent noiseGate { "D7S NoiseGT1" }; RackModuleComponent eq4k { "D7S EQ 4K" }; RackModuleComponent comp76 { "D7S 76" }; RackModuleComponent comp2a { "D7S 2A" }; RackModuleComponent tube { "D7S Tube" }; RackModuleComponent clipper { "D7S Clipper" }; RackModuleComponent esser { "D7S Esser" }; RackModuleComponent delay { "D7S Delay" };
    ParamSlider rackInput; ParamSlider rackOutput; ParamSlider rackMix; juce::Label rackMeterLabel; HorizontalMeter rackInMeter; HorizontalMeter rackOutMeter; std::unique_ptr<SpectrumAnalyzerComponent> analyzerView;
    ParamSlider noiseSuppression; juce::Label noiseMeterLabel; HorizontalMeter noiseGrMeter; juce::ToggleButton noiseBypassButton; std::unique_ptr<ButtonAttachment> noiseBypassAttachment;
    ParamSlider eqHpf; ParamSlider eqLpf; ParamSlider eqLfFreq; ParamSlider eqLfGain; ParamSlider eqLmfFreq; ParamSlider eqLmfGain; ParamSlider eqLmfQ; ParamSlider eqHmfFreq; ParamSlider eqHmfGain; ParamSlider eqHmfQ; ParamSlider eqHfFreq; ParamSlider eqHfGain; ParamSlider eqDrive; juce::ToggleButton eqLfBellButton; juce::ToggleButton eqHfBellButton; juce::ToggleButton eqBypassButton; std::unique_ptr<ButtonAttachment> eqLfBellAttachment; std::unique_ptr<ButtonAttachment> eqHfBellAttachment; std::unique_ptr<ButtonAttachment> eqBypassAttachment;
    ParamSlider comp76Input; ParamSlider comp76Output; ParamSlider comp76Attack; ParamSlider comp76Release; std::array<juce::TextButton, 5> comp76RatioButtons { juce::TextButton { "4" }, juce::TextButton { "8" }, juce::TextButton { "12" }, juce::TextButton { "20" }, juce::TextButton { "All" } }; juce::Label comp76MeterLabel; HorizontalMeter comp76GrMeter; juce::ToggleButton comp76BypassButton; std::unique_ptr<ButtonAttachment> comp76BypassAttachment;
    ParamSlider comp2aPeak; ParamSlider comp2aGain; ParamSlider comp2aEmphasis; ParamSlider comp2aMix; std::array<juce::TextButton, 2> comp2aModeButtons { juce::TextButton { "Comp" }, juce::TextButton { "Limit" } }; juce::Label comp2aMeterLabel; HorizontalMeter comp2aGrMeter; juce::ToggleButton comp2aBypassButton; std::unique_ptr<ButtonAttachment> comp2aBypassAttachment;
    ParamSlider tubeBeauty; ParamSlider tubeBeast; ParamSlider tubeSensitivity; ParamSlider tubeMix; juce::ToggleButton tubeBypassButton; std::unique_ptr<ButtonAttachment> tubeBypassAttachment; ParamSlider clipperPre; ParamSlider clipperThreshold; ParamSlider clipperPost; juce::ToggleButton clipperBypassButton; std::unique_ptr<ButtonAttachment> clipperBypassAttachment;
    ParamSlider esserThreshold; ParamSlider esserFreq; ParamSlider esserRange; std::array<juce::TextButton, 2> esserModeButtons { juce::TextButton { "Wide" }, juce::TextButton { "Split" } }; juce::Label esserMeterLabel; HorizontalMeter esserGrMeter; juce::ToggleButton esserBypassButton; std::unique_ptr<ButtonAttachment> esserBypassAttachment;
    ParamSlider delayMix; ParamSlider delayFeedback; ParamSlider delayGlideTime; ParamSlider delayTimeKnob; juce::ComboBox delayModeBox; juce::Label delayModeLabel; juce::Label delayTimeValueLabel; std::unique_ptr<ComboBoxAttachment> delayModeAttachment; std::unique_ptr<SliderAttachment> delayFractionAttachment; std::unique_ptr<SliderAttachment> delayTimeMsAttachment; std::array<juce::TextButton, 3> delayDirectionButtons { juce::TextButton { "Up" }, juce::TextButton { "Down" }, juce::TextButton { "Random" } }; juce::ToggleButton delayGlideButton; juce::ToggleButton delayBypassButton; std::unique_ptr<ButtonAttachment> delayGlideAttachment; std::unique_ptr<ButtonAttachment> delayBypassAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessorEditor)
};
