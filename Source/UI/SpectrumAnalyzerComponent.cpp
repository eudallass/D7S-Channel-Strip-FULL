#include "SpectrumAnalyzerComponent.h"

namespace
{
    constexpr float majorFrequencies[] = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
                                           1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };

    constexpr float minorFrequencies[] = { 30.0f, 40.0f, 60.0f, 80.0f, 300.0f, 400.0f,
                                           600.0f, 800.0f, 3000.0f, 4000.0f, 6000.0f,
                                           8000.0f, 12000.0f, 16000.0f };

    constexpr int majorDbLines[] = { -100, -80, -60, -40, -20, 0 };
    constexpr int minorDbLines[] = { -90, -70, -50, -30, -10 };

    juce::String frequencyLabel (float hz)
    {
        if (hz >= 1000.0f)
        {
            const float k = hz / 1000.0f;
            return juce::approximatelyEqual (std::round (k), k)
                     ? juce::String ((int) std::round (k)) + "k"
                     : juce::String (k, 1) + "k";
        }

        return juce::String ((int) std::round (hz));
    }
}

SpectrumAnalyzerComponent::SpectrumAnalyzerComponent (SpectrumAnalyzer& a, SpectrumAnalyzer& b)
    : pre (a), post (b)
{
    setOpaque (false);
    setInterceptsMouseClicks (true, true);

    for (auto* button : { &preButton, &postButton, &speedButton, &smoothButton, &tiltButton, &rangeButton, &holdButton })
    {
        setupButton (*button);
        addAndMakeVisible (*button);
    }

    preButton.onClick = [this]
    {
        setShowPre (! showPre);
    };

    postButton.onClick = [this]
    {
        setShowPost (! showPost);
    };

    speedButton.onClick = [this]
    {
        if (speedMode == SpeedMode::Fast)           speedMode = SpeedMode::Medium;
        else if (speedMode == SpeedMode::Medium)   speedMode = SpeedMode::Slow;
        else                                       speedMode = SpeedMode::Fast;

        applySpeedMode();
    };

    smoothButton.onClick = [this]
    {
        if (smoothMode == SmoothMode::Detailed)       smoothMode = SmoothMode::Normal;
        else if (smoothMode == SmoothMode::Normal)    smoothMode = SmoothMode::Smooth;
        else                                         smoothMode = SmoothMode::Detailed;

        applySmoothMode();
    };

    tiltButton.onClick = [this]
    {
        tiltEnabled = ! tiltEnabled;
        applyTiltMode();
    };

    rangeButton.onClick = [this]
    {
        if (rangeMode == RangeMode::Full)       rangeMode = RangeMode::Mix;
        else if (rangeMode == RangeMode::Mix)   rangeMode = RangeMode::Focus;
        else                                   rangeMode = RangeMode::Full;

        applyRangeMode();
    };

    holdButton.onClick = [this]
    {
        frozen = ! frozen;
        updateButtonText();
        repaint();
    };

    applySpeedMode();
    applySmoothMode();
    applyTiltMode();
    applyRangeMode();
    startTimerHz (30);
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
    stopTimer();
}

void SpectrumAnalyzerComponent::setShowPre (bool shouldShow) noexcept
{
    showPre = shouldShow;
    updateButtonText();
    repaint();
}

void SpectrumAnalyzerComponent::setShowPost (bool shouldShow) noexcept
{
    showPost = shouldShow;
    updateButtonText();
    repaint();
}

void SpectrumAnalyzerComponent::setupButton (juce::TextButton& button)
{
    button.setWantsKeyboardFocus (false);
    button.setTriggeredOnMouseDown (false);
    button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff181818));
    button.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff222222));
    button.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffb8b8b8));
    button.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff00d4ff));
}

void SpectrumAnalyzerComponent::updateButtonText()
{
    preButton.setButtonText (showPre ? "PRE" : "pre");
    postButton.setButtonText (showPost ? "POST" : "post");

    if (speedMode == SpeedMode::Fast)          speedButton.setButtonText ("FAST");
    else if (speedMode == SpeedMode::Medium)  speedButton.setButtonText ("MED");
    else                                      speedButton.setButtonText ("SLOW");

    if (smoothMode == SmoothMode::Detailed)       smoothButton.setButtonText ("1/48");
    else if (smoothMode == SmoothMode::Normal)    smoothButton.setButtonText ("1/24");
    else                                         smoothButton.setButtonText ("1/12");

    tiltButton.setButtonText (tiltEnabled ? "TILT" : "FLAT");

    if (rangeMode == RangeMode::Full)        rangeButton.setButtonText ("FULL");
    else if (rangeMode == RangeMode::Mix)    rangeButton.setButtonText ("MIX");
    else                                    rangeButton.setButtonText ("FOCUS");

    holdButton.setButtonText (frozen ? "HELD" : "HOLD");
}

void SpectrumAnalyzerComponent::applySpeedMode()
{
    if (speedMode == SpeedMode::Fast)
    {
        pre.setDecayDbPerSecond (55.0f);
        post.setDecayDbPerSecond (55.0f);
        pre.setTemporalSmoothingMs (35.0f);
        post.setTemporalSmoothingMs (35.0f);
    }
    else if (speedMode == SpeedMode::Medium)
    {
        pre.setDecayDbPerSecond (30.0f);
        post.setDecayDbPerSecond (30.0f);
        pre.setTemporalSmoothingMs (75.0f);
        post.setTemporalSmoothingMs (75.0f);
    }
    else
    {
        pre.setDecayDbPerSecond (16.0f);
        post.setDecayDbPerSecond (16.0f);
        pre.setTemporalSmoothingMs (150.0f);
        post.setTemporalSmoothingMs (150.0f);
    }

    updateButtonText();
}

void SpectrumAnalyzerComponent::applySmoothMode()
{
    if (smoothMode == SmoothMode::Detailed)
    {
        pre.setSmoothingOctaveFraction (1.0f / 48.0f);
        post.setSmoothingOctaveFraction (1.0f / 48.0f);
        pre.setPeakBlend (0.75f);
        post.setPeakBlend (0.75f);
    }
    else if (smoothMode == SmoothMode::Normal)
    {
        pre.setSmoothingOctaveFraction (1.0f / 24.0f);
        post.setSmoothingOctaveFraction (1.0f / 24.0f);
        pre.setPeakBlend (0.62f);
        post.setPeakBlend (0.62f);
    }
    else
    {
        pre.setSmoothingOctaveFraction (1.0f / 12.0f);
        post.setSmoothingOctaveFraction (1.0f / 12.0f);
        pre.setPeakBlend (0.45f);
        post.setPeakBlend (0.45f);
    }

    updateButtonText();
}

void SpectrumAnalyzerComponent::applyTiltMode()
{
    pre.setTiltDbPerOctave (tiltEnabled ? 4.5f : 0.0f);
    post.setTiltDbPerOctave (tiltEnabled ? 4.5f : 0.0f);
    updateButtonText();
}

void SpectrumAnalyzerComponent::applyRangeMode()
{
    if (rangeMode == RangeMode::Full)
    {
        pre.setDecibelRange (-100.0f, 6.0f);
        post.setDecibelRange (-100.0f, 6.0f);
    }
    else if (rangeMode == RangeMode::Mix)
    {
        pre.setDecibelRange (-72.0f, 6.0f);
        post.setDecibelRange (-72.0f, 6.0f);
    }
    else
    {
        pre.setDecibelRange (-48.0f, 6.0f);
        post.setDecibelRange (-48.0f, 6.0f);
    }

    buildPath (prePath,  pre.getDisplayData());
    buildPath (postPath, post.getDisplayData());
    updateButtonText();
    repaint();
}

void SpectrumAnalyzerComponent::timerCallback()
{
    if (frozen)
        return;

    bool dirty = false;
    if (pre.processFFTIfReady())  dirty = true;
    if (post.processFFTIfReady()) dirty = true;

    if (dirty)
    {
        if (showPre)  buildPath (prePath,  pre.getDisplayData());
        if (showPost) buildPath (postPath, post.getDisplayData());
        repaint();
    }
}

float SpectrumAnalyzerComponent::yFromDb (float db) const noexcept
{
    const float minDb = post.getMinDb();
    const float maxDb = post.getMaxDb();
    const float norm  = 1.0f - juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
    return norm * (float) getHeight();
}

float SpectrumAnalyzerComponent::xFromHz (float hz) const noexcept
{
    const float logMin = std::log (post.getMinFreq());
    const float logMax = std::log (post.getMaxFreq());
    const float clampedHz = juce::jlimit (post.getMinFreq(), post.getMaxFreq(), hz);
    const float t = (std::log (clampedHz) - logMin) / (logMax - logMin);
    return t * (float) getWidth();
}

void SpectrumAnalyzerComponent::buildPath (juce::Path& path,
    const std::array<float, SpectrumAnalyzer::scopeSize>& data) const
{
    path.clear();
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    const float logMin = std::log (post.getMinFreq());
    const float logMax = std::log (post.getMaxFreq());

    for (size_t i = 0; i < data.size(); ++i)
    {
        const float t = (float) i / (float) (data.size() - 1);
        const float hz = std::exp (logMin + t * (logMax - logMin));
        const float x = xFromHz (hz);
        const float y = yFromDb (data[i]);

        if (i == 0) path.startNewSubPath (x, y);
        else        path.lineTo (x, y);
    }
}

void SpectrumAnalyzerComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient bg (juce::Colour (0xff111111), bounds.getTopLeft(),
                             juce::Colour (0xff070707), bounds.getBottomRight(), false);
    g.setGradientFill (bg);
    g.fillRect (bounds);

    g.setColour (juce::Colour (0xff202020));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

    g.setColour (juce::Colour (0x332f2f2f));
    for (int db : minorDbLines)
        g.drawHorizontalLine ((int) yFromDb ((float) db), bounds.getX(), bounds.getRight());

    g.setColour (juce::Colour (0x552f2f2f));
    for (int db : majorDbLines)
        g.drawHorizontalLine ((int) yFromDb ((float) db), bounds.getX(), bounds.getRight());

    g.setColour (juce::Colour (0x302f2f2f));
    for (float hz : minorFrequencies)
        g.drawVerticalLine ((int) xFromHz (hz), bounds.getY(), bounds.getBottom());

    g.setColour (juce::Colour (0x552f2f2f));
    for (float hz : majorFrequencies)
        g.drawVerticalLine ((int) xFromHz (hz), bounds.getY(), bounds.getBottom());

    g.setFont (juce::Font (10.0f, juce::Font::plain));
    g.setColour (juce::Colour (0xff777777));

    for (int db : majorDbLines)
    {
        const int y = (int) yFromDb ((float) db);
        g.drawText (juce::String (db) + " dB", 5, y - 7, 46, 14,
                    juce::Justification::left, false);
    }

    for (float hz : majorFrequencies)
    {
        const int x = (int) xFromHz (hz);
        g.drawText (frequencyLabel (hz), x - 18, getHeight() - 16, 36, 13,
                    juce::Justification::centred, false);
    }

    g.setFont (juce::Font (11.0f, juce::Font::plain));
    g.setColour (juce::Colour (0xff8a8a8a));
    g.drawText ("D7S ANALYZER", 8, 6, 110, 16, juce::Justification::left, false);

    if (frozen)
    {
        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xffffc857));
        g.drawText ("HOLD", getWidth() - 50, 6, 42, 16, juce::Justification::right, false);
    }

    if (showPre && ! prePath.isEmpty())
    {
        juce::Path filled = prePath;
        filled.lineTo (bounds.getRight(), bounds.getBottom());
        filled.lineTo (bounds.getX(),     bounds.getBottom());
        filled.closeSubPath();

        g.setColour (juce::Colour (0x2ea0a0a0));
        g.fillPath (filled);

        g.setColour (juce::Colour (0xff747474));
        g.strokePath (prePath, juce::PathStrokeType (1.0f,
                                                     juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    if (showPost && ! postPath.isEmpty())
    {
        g.setColour (juce::Colour (0x3300d4ff));
        g.strokePath (postPath, juce::PathStrokeType (4.0f,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        g.setColour (juce::Colour (0xff00d4ff));
        g.strokePath (postPath, juce::PathStrokeType (1.6f,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }
}

void SpectrumAnalyzerComponent::resized()
{
    auto top = getLocalBounds().reduced (8, 6).removeFromTop (18);
    top.removeFromLeft (118);

    const int smallW = 48;
    const int wideW = 54;
    const int gap = 5;

    preButton.setBounds (top.removeFromLeft (smallW));
    top.removeFromLeft (gap);
    postButton.setBounds (top.removeFromLeft (smallW));
    top.removeFromLeft (gap);
    speedButton.setBounds (top.removeFromLeft (smallW));
    top.removeFromLeft (gap);
    smoothButton.setBounds (top.removeFromLeft (smallW));
    top.removeFromLeft (gap);
    tiltButton.setBounds (top.removeFromLeft (smallW));
    top.removeFromLeft (gap);
    rangeButton.setBounds (top.removeFromLeft (wideW));
    top.removeFromLeft (gap);
    holdButton.setBounds (top.removeFromLeft (wideW));

    buildPath (prePath,  pre.getDisplayData());
    buildPath (postPath, post.getDisplayData());
}
