#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Core/SpectrumAnalyzer.h"

/**
 *  Pro-Q 4 style display: pre-process curve in translucent grey (filled below),
 *  post-process curve in bright cyan on top. 30 fps timer drives FFT consumption
 *  and repaints. Background grid shows dB and frequency reference lines.
 */
class SpectrumAnalyzerComponent : public juce::Component,
                                  private juce::Timer
{
public:
    SpectrumAnalyzerComponent (SpectrumAnalyzer& preAnalyzer,
                               SpectrumAnalyzer& postAnalyzer);
    ~SpectrumAnalyzerComponent() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

    void setShowPre  (bool shouldShow) noexcept { showPre  = shouldShow; repaint(); }
    void setShowPost (bool shouldShow) noexcept { showPost = shouldShow; repaint(); }

private:
    void timerCallback() override;
    void buildPath (juce::Path& path,
                    const std::array<float, SpectrumAnalyzer::scopeSize>& data) const;
    float yFromDb (float db) const noexcept;
    float xFromHz (float hz) const noexcept;

    SpectrumAnalyzer& pre;
    SpectrumAnalyzer& post;
    juce::Path prePath, postPath;
    bool showPre  = true;
    bool showPost = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzerComponent)
};
