#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Core/SpectrumAnalyzer.h"

class SpectrumAnalyzerComponent : public juce::Component,
                                  private juce::Timer
{
public:
    SpectrumAnalyzerComponent (SpectrumAnalyzer& preAnalyzer,
                               SpectrumAnalyzer& postAnalyzer);
    ~SpectrumAnalyzerComponent() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

    void setShowPre  (bool shouldShow) noexcept;
    void setShowPost (bool shouldShow) noexcept;

private:
    enum class SpeedMode { Fast, Medium, Slow };
    enum class SmoothMode { Detailed, Normal, Smooth };

    void timerCallback() override;
    void buildPath (juce::Path& path,
                    const std::array<float, SpectrumAnalyzer::scopeSize>& data) const;
    float yFromDb (float db) const noexcept;
    float xFromHz (float hz) const noexcept;

    void setupButton (juce::TextButton& button);
    void updateButtonText();
    void applySpeedMode();
    void applySmoothMode();
    void applyTiltMode();

    SpectrumAnalyzer& pre;
    SpectrumAnalyzer& post;
    juce::Path prePath, postPath;

    juce::TextButton preButton   { "PRE" };
    juce::TextButton postButton  { "POST" };
    juce::TextButton speedButton { "SPD" };
    juce::TextButton smoothButton { "SMTH" };
    juce::TextButton tiltButton  { "TILT" };

    bool showPre  = true;
    bool showPost = true;
    bool tiltEnabled = true;
    SpeedMode speedMode = SpeedMode::Medium;
    SmoothMode smoothMode = SmoothMode::Normal;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzerComponent)
};
