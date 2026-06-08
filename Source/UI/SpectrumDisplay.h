#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include "../PluginProcessor.h"

class SpectrumDisplay final : public juce::Component,
                              private juce::Timer
{
public:
    explicit SpectrumDisplay (D7SChannelStripFullAudioProcessor& processorToUse);
    ~SpectrumDisplay() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    D7SChannelStripFullAudioProcessor& processor;

    juce::TextButton preButton { "Pre" };
    juce::TextButton postButton { "Post" };
    juce::TextButton autoButton { "Auto" };
    juce::TextButton freezeButton { "Freeze" };
    juce::TextButton settingsButton { "⚙" };

    std::unique_ptr<ButtonAttachment> preAttachment;
    std::unique_ptr<ButtonAttachment> postAttachment;
    std::unique_ptr<ButtonAttachment> autoAttachment;
    std::unique_ptr<ButtonAttachment> freezeAttachment;

    bool mouseInside { false };
    juce::Point<float> mousePosition;

    float preAlpha { 1.0f };
    float postAlpha { 1.0f };
    juce::SmoothedValue<float> preAlphaSmooth;
    juce::SmoothedValue<float> postAlphaSmooth;

    void timerCallback() override;
    void updateButtonColours();
    void showSettingsMenu();

    float frequencyToX (float hz, juce::Rectangle<float> plot) const noexcept;
    float binToFrequency (int bin) const noexcept;
    float dbToY (float dbRelative, juce::Rectangle<float> plot) const noexcept;
    float rangeDb() const noexcept;
    float maxRelativeDb() const noexcept;
    float minRelativeDb() const noexcept;
    float displayDbForBin (float dbFs, float freqHz) const noexcept;

    std::vector<juce::Point<float>> collectSpectrumPoints (bool post, juce::Rectangle<float> plot) const;
    juce::Path buildSmoothedPath (const std::vector<juce::Point<float>>& points, bool closeToBottom, juce::Rectangle<float> plot) const;
    juce::Path buildSpectrumPath (bool post, juce::Rectangle<float> plot, bool closeToBottom) const;
    void drawGrid (juce::Graphics& g, juce::Rectangle<float> plot);
    void drawCurve (juce::Graphics& g, bool post, juce::Colour colour, float alpha, juce::Rectangle<float> plot);
    void drawPeakLabels (juce::Graphics& g, juce::Rectangle<float> plot);
    void drawHover (juce::Graphics& g, juce::Rectangle<float> plot);
};
