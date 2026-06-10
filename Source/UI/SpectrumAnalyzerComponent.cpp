#include "SpectrumAnalyzerComponent.h"

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
    const float t      = (std::log (juce::jlimit (post.getMinFreq(), post.getMaxFreq(), hz)) - logMin)
                       / (logMax - logMin);
    return t * (float) getWidth();
}

void SpectrumAnalyzerComponent::buildPath (juce::Path& path,
    const std::array<float, SpectrumAnalyzer::scopeSize>& data) const
{
    path.clear();
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    const float w     = (float) getWidth();
    const float minDb = post.getMinDb();
    const float maxDb = post.getMaxDb();
    const float h     = (float) getHeight();

    auto yFromDbLocal = [=] (float db)
    {
        const float norm = 1.0f - juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
        return norm * h;
    };

    for (size_t i = 0; i < data.size(); ++i)
    {
        const float x = (float) i / (float) (data.size() - 1) * w;
        const float y = yFromDbLocal (data[i]);
        if (i == 0) path.startNewSubPath (x, y);
        else        path.lineTo (x, y);
    }
}

void SpectrumAnalyzerComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff141414));
    g.fillRect (bounds);

    g.setColour (juce::Colour (0xff262626));
    for (int db : { -80, -60, -40, -20, 0 })
        g.drawHorizontalLine ((int) yFromDb ((float) db), bounds.getX(), bounds.getRight());

    for (float hz : { 100.0f, 1000.0f, 10000.0f })
        g.drawVerticalLine ((int) xFromHz (hz), bounds.getY(), bounds.getBottom());

    g.setColour (juce::Colour (0xff5a5a5a));
    g.setFont (juce::Font (10.0f));
    for (int db : { -80, -60, -40, -20, 0 })
        g.drawText (juce::String (db) + " dB", 4, (int) yFromDb ((float) db) - 10, 40, 12,
                    juce::Justification::left, false);

    for (auto pr : std::initializer_list<std::pair<float, const char*>> {
            { 100.0f, "100" }, { 1000.0f, "1k" }, { 10000.0f, "10k" } })
    {
        g.drawText (pr.second, (int) xFromHz (pr.first) - 12, getHeight() - 14, 28, 12,
                    juce::Justification::centred, false);
    }

    if (showPre && ! prePath.isEmpty())
    {
        juce::Path filled = prePath;
        filled.lineTo (bounds.getRight(), bounds.getBottom());
        filled.lineTo (bounds.getX(),     bounds.getBottom());
        filled.closeSubPath();

        g.setColour (juce::Colour (0x33a0a0a0));
        g.fillPath (filled);

        g.setColour (juce::Colour (0xff707070));
        g.strokePath (prePath, juce::PathStrokeType (1.0f));
    }

    if (showPost && ! postPath.isEmpty())
    {
        g.setColour (juce::Colour (0xff00d4ff));
        g.strokePath (postPath, juce::PathStrokeType (1.5f,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }
}

void SpectrumAnalyzerComponent::resized()
{
}
