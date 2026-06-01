#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class RackModuleComponent : public juce::Component
{
public:
    explicit RackModuleComponent (juce::String moduleName)
        : name (std::move (moduleName))
    {
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();

        g.setColour (juce::Colour (45, 45, 45));
        g.fillRoundedRectangle (area.reduced (1.0f), 8.0f);

        g.setColour (bypassed ? juce::Colours::darkred : juce::Colours::silver);
        g.drawRoundedRectangle (area.reduced (1.0f), 8.0f, 2.0f);

        g.setColour (juce::Colours::white);
        g.setFont (22.0f);
        g.drawText (name, getLocalBounds().reduced (20, 0), juce::Justification::centredLeft);

        auto buttonArea = getLocalBounds().removeFromRight (110).reduced (20, 18);

        g.setColour (bypassed ? juce::Colours::darkred : juce::Colour (25, 90, 45));
        g.fillRoundedRectangle (buttonArea.toFloat(), 6.0f);

        g.setColour (juce::Colours::white);
        g.setFont (14.0f);
        g.drawText (bypassed ? "BYPASS" : "ACTIVE", buttonArea, juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        auto buttonArea = getLocalBounds().removeFromRight (110).reduced (20, 18);

        if (buttonArea.contains (event.getPosition()))
        {
            bypassed = ! bypassed;
            repaint();
        }
    }

private:
    juce::String name;
    bool bypassed = false;
};
