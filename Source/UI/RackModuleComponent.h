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

        g.setColour (juce::Colours::silver);
        g.drawRoundedRectangle (area.reduced (1.0f), 8.0f, 2.0f);

        g.setColour (juce::Colours::white);
        g.setFont (22.0f);
        g.drawText (name, getLocalBounds().reduced (20, 0), juce::Justification::centredLeft);
    }

private:
    juce::String name;
};
