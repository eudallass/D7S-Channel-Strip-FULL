#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class RackModuleComponent : public juce::Component
{
public:
    explicit RackModuleComponent (juce::String moduleName)
        : name (std::move (moduleName))
    {
    }

    void setEnabledState (bool shouldBeEnabled)
    {
        enabled = shouldBeEnabled;
        repaint();
    }

    bool isEnabledState() const
    {
        return enabled;
    }

    std::function<void(bool)> onEnabledChanged;
    std::function<void(RackModuleComponent*, const juce::MouseEvent&)> onDragStart;
    std::function<void(RackModuleComponent*, const juce::MouseEvent&)> onDragMove;
    std::function<void(RackModuleComponent*, const juce::MouseEvent&)> onDragEnd;

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced (1.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.35f), 8, { 0, 2 }).drawForRectangle (g, area.toNearestInt());

        juce::ColourGradient panelGrad (juce::Colour (48, 50, 55), area.getX(), area.getY(),
                                        juce::Colour (25, 27, 31), area.getX(), area.getBottom(), false);
        g.setGradientFill (panelGrad);
        g.fillRoundedRectangle (area, 8.0f);

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (area.removeFromTop (area.getHeight() * 0.42f), 8.0f);

        const auto outline = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (enabled ? juce::Colour (255, 180, 60).withAlpha (0.72f)
                             : juce::Colour (140, 54, 54).withAlpha (0.80f));
        g.drawRoundedRectangle (outline, 8.0f, enabled ? 1.6f : 1.2f);

        g.setColour (enabled ? juce::Colours::white : juce::Colours::grey);
        g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
        g.drawText (name, getLocalBounds().reduced (18, 0), juce::Justification::centredLeft);

        auto buttonArea = getBypassButtonArea().toFloat();
        const auto buttonColour = enabled ? juce::Colour (30, 105, 54) : juce::Colour (105, 32, 32);
        g.setColour (buttonColour);
        g.fillRoundedRectangle (buttonArea, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.drawRoundedRectangle (buttonArea, 6.0f, 1.0f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (enabled ? "ACTIVE" : "BYPASS", buttonArea.toNearestInt(), juce::Justification::centred);

        auto grip = getLocalBounds().removeFromLeft (12).reduced (4, 14);
        g.setColour (juce::Colours::white.withAlpha (0.34f));
        for (int y = grip.getY(); y < grip.getBottom(); y += 6)
        {
            g.fillEllipse ((float) grip.getX(), (float) y, 3.0f, 3.0f);
            g.fillEllipse ((float) grip.getX() + 5.0f, (float) y, 3.0f, 3.0f);
        }
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        if (getBypassButtonArea().contains (event.getPosition()))
        {
            enabled = ! enabled;

            if (onEnabledChanged)
                onEnabledChanged (enabled);

            repaint();
            return;
        }

        beginDragAutoRepeat (50);
        draggingHeader = true;
        if (onDragStart)
            onDragStart (this, event);
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        if (! draggingHeader)
            return;

        if (auto* viewport = findParentComponentOfClass<juce::Viewport>())
        {
            const auto pos = viewport->getLocalPoint (this, event.getPosition());
            viewport->autoScroll (pos.x, pos.y, 50, 8);
        }

        if (onDragMove)
            onDragMove (this, event);
    }

    void mouseUp (const juce::MouseEvent& event) override
    {
        if (draggingHeader && onDragEnd)
            onDragEnd (this, event);
        draggingHeader = false;
    }

private:
    juce::Rectangle<int> getBypassButtonArea() const
    {
        return getLocalBounds().removeFromRight (92).reduced (14, 17);
    }

    juce::String name;
    bool enabled = true;
    bool draggingHeader = false;
};
