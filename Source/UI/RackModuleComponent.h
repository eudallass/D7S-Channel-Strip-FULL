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
        auto area = getLocalBounds().toFloat();

        g.setColour (juce::Colour (45, 45, 45));
        g.fillRoundedRectangle (area.reduced (1.0f), 8.0f);

        g.setColour (enabled ? juce::Colours::silver : juce::Colours::darkred);
        g.drawRoundedRectangle (area.reduced (1.0f), 8.0f, 2.0f);

        g.setColour (enabled ? juce::Colours::white : juce::Colours::grey);
        g.setFont (19.0f);
        g.drawText (name, getLocalBounds().reduced (16, 0), juce::Justification::centredLeft);

        auto buttonArea = getBypassButtonArea();

        g.setColour (enabled ? juce::Colour (25, 90, 45) : juce::Colours::darkred);
        g.fillRoundedRectangle (buttonArea.toFloat(), 6.0f);

        g.setColour (juce::Colours::white);
        g.setFont (13.0f);
        g.drawText (enabled ? "ACTIVE" : "BYPASS", buttonArea, juce::Justification::centred);

        auto grip = getLocalBounds().removeFromLeft (10).reduced (3, 15);
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        for (int y = grip.getY(); y < grip.getBottom(); y += 6)
            g.fillEllipse ((float) grip.getX(), (float) y, 3.0f, 3.0f);
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
