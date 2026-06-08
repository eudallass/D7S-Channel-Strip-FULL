#include "SpectrumDisplay.h"

namespace
{
    static juce::String formatFrequency (float hz)
    {
        if (hz >= 1000.0f)
            return juce::String (hz / 1000.0f, 1) + " kHz";
        return juce::String ((int) std::round (hz)) + " Hz";
    }

    static juce::String formatDb (float db)
    {
        return (db >= 0.0f ? "+" : "") + juce::String (db, 1) + " dB";
    }
}

SpectrumDisplay::SpectrumDisplay (D7SChannelStripFullAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    addAndMakeVisible (preButton);
    addAndMakeVisible (postButton);
    addAndMakeVisible (autoButton);
    addAndMakeVisible (freezeButton);
    addAndMakeVisible (settingsButton);

    auto& apvts = processor.getAPVTS();
    preAttachment = std::make_unique<ButtonAttachment> (apvts, "analyzer_pre_enabled", preButton);
    postAttachment = std::make_unique<ButtonAttachment> (apvts, "analyzer_post_enabled", postButton);
    autoAttachment = std::make_unique<ButtonAttachment> (apvts, "analyzer_auto_range", autoButton);
    freezeAttachment = std::make_unique<ButtonAttachment> (apvts, "analyzer_freeze", freezeButton);

    settingsButton.onClick = [this] { showSettingsMenu(); };

    preAlphaSmooth.reset (30.0, 0.150);
    postAlphaSmooth.reset (30.0, 0.150);
    preAlphaSmooth.setCurrentAndTargetValue (1.0f);
    postAlphaSmooth.setCurrentAndTargetValue (1.0f);

    startTimerHz (33);
}

SpectrumDisplay::~SpectrumDisplay()
{
    stopTimer();
}

void SpectrumDisplay::resized()
{
    auto r = getLocalBounds().reduced (8);
    auto top = r.removeFromTop (24);
    preButton.setBounds (top.removeFromLeft (52)); top.removeFromLeft (5);
    postButton.setBounds (top.removeFromLeft (56)); top.removeFromLeft (5);
    autoButton.setBounds (top.removeFromLeft (56));
    settingsButton.setBounds (r.getRight() - 30, r.getY() - 24, 28, 22);
    freezeButton.setBounds (r.getX(), r.getBottom() - 26, 78, 22);
}

void SpectrumDisplay::timerCallback()
{
    const auto& s = processor.getAnalyzerState();
    preAlphaSmooth.setTargetValue (s.preEnabled.load() ? 1.0f : 0.0f);
    postAlphaSmooth.setTargetValue (s.postEnabled.load() ? 1.0f : 0.0f);
    preAlpha = preAlphaSmooth.getNextValue();
    postAlpha = postAlphaSmooth.getNextValue();
    updateButtonColours();
    repaint();
}

void SpectrumDisplay::updateButtonColours()
{
    const auto& s = processor.getAnalyzerState();
    auto setButton = [] (juce::TextButton& b, bool on, juce::Colour c)
    {
        b.setColour (juce::TextButton::buttonColourId, on ? c : juce::Colour (40, 40, 40));
        b.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        b.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    };

    setButton (preButton, s.preEnabled.load(), juce::Colour (70, 140, 230));
    setButton (postButton, s.postEnabled.load(), juce::Colour (230, 80, 80));
    setButton (autoButton, s.autoRange.load(), juce::Colour (255, 180, 60));
    freezeButton.setColour (juce::TextButton::buttonColourId, s.freeze.load() ? juce::Colour (255, 180, 60) : juce::Colour (40, 40, 40));
    freezeButton.setColour (juce::TextButton::textColourOffId, s.freeze.load() ? juce::Colours::black : juce::Colours::white);
}

void SpectrumDisplay::showSettingsMenu()
{
    juce::PopupMenu resolution;
    resolution.addItem (100, "Low");
    resolution.addItem (101, "Medium");
    resolution.addItem (102, "High");
    resolution.addItem (103, "Maximum");

    juce::PopupMenu speed;
    speed.addItem (200, "Very Fast");
    speed.addItem (201, "Fast");
    speed.addItem (202, "Medium");
    speed.addItem (203, "Slow");
    speed.addItem (204, "Very Slow");

    juce::PopupMenu range;
    range.addItem (300, "60 dB");
    range.addItem (301, "90 dB");
    range.addItem (302, "120 dB");

    juce::PopupMenu menu;
    menu.addSectionHeader ("Analyzer Settings");
    menu.addSubMenu ("Resolution", resolution);
    menu.addSubMenu ("Speed", speed);
    menu.addSubMenu ("Range", range);
    menu.addSeparator();
    menu.addItem (400, "Freeze", true, processor.getAnalyzerState().freeze.load());

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsButton), [this] (int result)
    {
        auto& apvts = processor.getAPVTS();
        auto setNormalised = [&apvts] (const char* id, float normalised)
        {
            if (auto* p = apvts.getParameter (id))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, normalised));
                p->endChangeGesture();
            }
        };
        auto toggleBool = [&apvts] (const char* id)
        {
            if (auto* p = apvts.getParameter (id))
            {
                const float next = p->getValue() > 0.5f ? 0.0f : 1.0f;
                p->beginChangeGesture();
                p->setValueNotifyingHost (next);
                p->endChangeGesture();
            }
        };

        if (result >= 100 && result <= 103) setNormalised ("analyzer_resolution", (float) (result - 100) / 3.0f);
        else if (result >= 200 && result <= 204) setNormalised ("analyzer_speed", (float) (result - 200) / 4.0f);
        else if (result >= 300 && result <= 302) setNormalised ("analyzer_range", (float) (result - 300) / 2.0f);
        else if (result == 400) toggleBool ("analyzer_freeze");
    });
}

float SpectrumDisplay::frequencyToX (float hz, juce::Rectangle<float> plot) const noexcept
{
    const float minHz = 20.0f;
    const float maxHz = 20000.0f;
    const float normalised = std::log (hz / minHz) / std::log (maxHz / minHz);
    return plot.getX() + plot.getWidth() * juce::jlimit (0.0f, 1.0f, normalised);
}

float SpectrumDisplay::binToFrequency (int bin) const noexcept
{
    const float minHz = 20.0f;
    const float maxHz = 20000.0f;
    const float normalised = (float) bin / (float) juce::jmax (1, AnalyzerState::numBins - 1);
    return minHz * std::pow (maxHz / minHz, normalised);
}

float SpectrumDisplay::rangeDb() const noexcept
{
    switch (processor.getAnalyzerState().rangeIndex.load())
    {
        case 0: return 60.0f;
        case 2: return 120.0f;
        default: return 90.0f;
    }
}

float SpectrumDisplay::maxRelativeDb() const noexcept
{
    return rangeDb() * 0.33333334f;
}

float SpectrumDisplay::minRelativeDb() const noexcept
{
    return maxRelativeDb() - rangeDb();
}

float SpectrumDisplay::dbToY (float dbRelative, juce::Rectangle<float> plot) const noexcept
{
    const float minDb = minRelativeDb();
    const float maxDb = maxRelativeDb();
    const float n = juce::jlimit (0.0f, 1.0f, (dbRelative - minDb) / (maxDb - minDb));
    return plot.getBottom() - n * plot.getHeight();
}

float SpectrumDisplay::displayDbForBin (float dbFs, float freqHz) const noexcept
{
    const auto& s = processor.getAnalyzerState();
    const float refDb = s.referenceDb.load();
    const float tilt = s.tiltDbPerOct.load();
    const float oct = std::log2 (juce::jmax (freqHz, 20.0f) / 1000.0f);
    return (dbFs - refDb) + tilt * oct;
}

std::vector<juce::Point<float>> SpectrumDisplay::collectSpectrumPoints (bool post, juce::Rectangle<float> plot) const
{
    std::vector<juce::Point<float>> points;
    points.reserve ((size_t) AnalyzerState::numBins);

    for (int i = 0; i < AnalyzerState::numBins; ++i)
    {
        const float hz = binToFrequency (i);
        const float dbFs = post ? processor.getPostSpectrumBinDb (i) : processor.getPreSpectrumBinDb (i);
        const float db = displayDbForBin (dbFs, hz);
        const float x = frequencyToX (hz, plot);
        const float y = dbToY (juce::jlimit (minRelativeDb(), maxRelativeDb(), db), plot);
        points.emplace_back (x, y);
    }

    return points;
}

juce::Path SpectrumDisplay::buildSmoothedPath (const std::vector<juce::Point<float>>& points, bool closeToBottom, juce::Rectangle<float> plot) const
{
    juce::Path path;
    if (points.empty())
        return path;

    path.startNewSubPath (points.front());

    if (points.size() == 1)
    {
        if (closeToBottom)
        {
            path.lineTo (points.front().x, plot.getBottom());
            path.closeSubPath();
        }
        return path;
    }

    for (size_t i = 0; i + 1 < points.size(); ++i)
    {
        const auto p0 = i == 0 ? points[i] : points[i - 1];
        const auto p1 = points[i];
        const auto p2 = points[i + 1];
        const auto p3 = (i + 2 < points.size()) ? points[i + 2] : points[i + 1];

        const auto c1 = p1 + (p2 - p0) / 6.0f;
        const auto c2 = p2 - (p3 - p1) / 6.0f;
        path.cubicTo (c1, c2, p2);
    }

    if (closeToBottom)
    {
        path.lineTo (points.back().x, plot.getBottom());
        path.lineTo (points.front().x, plot.getBottom());
        path.closeSubPath();
    }

    return path;
}

juce::Path SpectrumDisplay::buildSpectrumPath (bool post, juce::Rectangle<float> plot, bool closeToBottom) const
{
    return buildSmoothedPath (collectSpectrumPoints (post, plot), closeToBottom, plot);
}

void SpectrumDisplay::drawGrid (juce::Graphics& g, juce::Rectangle<float> plot)
{
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    const float freqs[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (float f : freqs)
    {
        const float x = frequencyToX (f, plot);
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.drawText (formatFrequency (f).replace (" Hz", ""), (int) x - 20, (int) plot.getBottom() + 2, 42, 14, juce::Justification::centred);
    }

    g.setColour (juce::Colour (255, 180, 60));
    g.drawLine (plot.getX(), dbToY (0.0f, plot), plot.getRight(), dbToY (0.0f, plot), 1.5f);

    g.setColour (juce::Colours::white.withAlpha (0.12f));
    const int step = rangeDb() > 100.0f ? 20 : 10;
    for (int db = 0; db >= (int) minRelativeDb(); db -= step)
    {
        const float y = dbToY ((float) db, plot);
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.drawText (formatDb ((float) db), (int) plot.getX() - 58, (int) y - 7, 52, 14, juce::Justification::centredRight);
        const float absDb = db + processor.getAnalyzerState().referenceDb.load();
        g.drawText (juce::String (absDb, 0), (int) plot.getRight() + 5, (int) y - 7, 42, 14, juce::Justification::centredLeft);
    }
}

void SpectrumDisplay::drawCurve (juce::Graphics& g, bool post, juce::Colour colour, float alpha, juce::Rectangle<float> plot)
{
    if (alpha <= 0.01f) return;
    auto fill = buildSpectrumPath (post, plot, true);
    auto line = buildSpectrumPath (post, plot, false);
    g.setColour (colour.withAlpha (0.235f * alpha));
    g.fillPath (fill);
    g.setColour (colour.withAlpha (0.95f * alpha));
    g.strokePath (line, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SpectrumDisplay::drawPeakLabels (juce::Graphics& g, juce::Rectangle<float> plot)
{
    g.setFont (10.5f);
    g.setColour (juce::Colour (255, 180, 60));
    int labels = 0;
    for (int i = 2; i < AnalyzerState::numBins - 2 && labels < 5; ++i)
    {
        const float hz = binToFrequency (i);
        const float db = displayDbForBin (processor.getPostSpectrumBinDb (i), hz);
        const float left = displayDbForBin (processor.getPostSpectrumBinDb (i - 1), binToFrequency (i - 1));
        const float right = displayDbForBin (processor.getPostSpectrumBinDb (i + 1), binToFrequency (i + 1));
        if (db > -40.0f && db > left + 3.0f && db > right + 3.0f)
        {
            const float x = frequencyToX (hz, plot);
            const float y = dbToY (juce::jlimit (minRelativeDb(), maxRelativeDb(), db), plot);
            juce::Path tri;
            tri.addTriangle (x, y - 8.0f, x - 4.0f, y - 2.0f, x + 4.0f, y - 2.0f);
            g.fillPath (tri);
            g.drawText (formatDb (db) + " @ " + formatFrequency (hz), (int) x + 5, (int) y - 18, 90, 14, juce::Justification::left);
            ++labels;
        }
    }
}

void SpectrumDisplay::drawHover (juce::Graphics& g, juce::Rectangle<float> plot)
{
    if (! mouseInside || ! plot.contains (mousePosition)) return;
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawVerticalLine ((int) mousePosition.x, plot.getY(), plot.getBottom());

    const float norm = (mousePosition.x - plot.getX()) / plot.getWidth();
    const float hz = 20.0f * std::pow (20000.0f / 20.0f, juce::jlimit (0.0f, 1.0f, norm));
    int bin = 0;
    float best = 1.0e9f;
    for (int i = 0; i < AnalyzerState::numBins; ++i)
    {
        const float d = std::abs (binToFrequency (i) - hz);
        if (d < best) { best = d; bin = i; }
    }

    const float pre = displayDbForBin (processor.getPreSpectrumBinDb (bin), binToFrequency (bin));
    const float post = displayDbForBin (processor.getPostSpectrumBinDb (bin), binToFrequency (bin));
    juce::String text = formatFrequency (hz) + "\nPre: " + formatDb (pre) + "\nPost: " + formatDb (post);
    auto box = juce::Rectangle<float> (mousePosition.x + 8.0f, plot.getY() + 8.0f, 118.0f, 52.0f);
    if (box.getRight() > plot.getRight()) box.setX (mousePosition.x - 126.0f);
    g.setColour (juce::Colour (15, 15, 18).withAlpha (0.92f));
    g.fillRoundedRectangle (box, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (11.0f);
    g.drawFittedText (text, box.toNearestInt().reduced (6), juce::Justification::centredLeft, 3);
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    auto plot = bounds.reduced (58.0f, 34.0f);
    plot.removeFromBottom (10.0f);

    g.setColour (juce::Colour (8, 10, 14));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (juce::Colour (70, 76, 86));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    drawGrid (g, plot);
    drawCurve (g, false, juce::Colour (70, 140, 230), preAlpha, plot);
    drawCurve (g, true, juce::Colour (230, 80, 80), postAlpha, plot);
    drawPeakLabels (g, plot);
    drawHover (g, plot);
}

void SpectrumDisplay::mouseMove (const juce::MouseEvent& e)
{
    mouseInside = true;
    mousePosition = e.position;
    repaint();
}

void SpectrumDisplay::mouseExit (const juce::MouseEvent&)
{
    mouseInside = false;
    repaint();
}
