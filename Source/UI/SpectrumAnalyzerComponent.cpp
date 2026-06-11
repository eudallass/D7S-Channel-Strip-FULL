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
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
    stopTimer();
}

void SpectrumAnalyzerComponent::timerCallback()
{
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

    // The analyzer core writes displayData in log-frequency order. Rebuilding the
    // frequency for each scope index here keeps the curve, grid and labels tied to
    // the same log-frequency mapping and avoids future linear/log mismatch bugs.
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
    buildPath (prePath,  pre.getDisplayData());
    buildPath (postPath, post.getDisplayData());
}
