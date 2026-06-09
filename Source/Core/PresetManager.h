#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    juce::File getPresetsDirectory() const;
    juce::File getFactoryDirectory() const;
    juce::StringArray getPresetNames() const;
    bool savePreset (const juce::String& name);
    bool loadPreset (const juce::String& name);
    bool deletePreset (const juce::String& name);
    bool renamePreset (const juce::String& oldName, const juce::String& newName);
    void installFactoryPresets();
    juce::String getCurrentPresetName() const { return currentPresetName; }
    bool isPresetDirty() const;

private:
    juce::File presetFileForName (const juce::String& name) const;
    static juce::String sanitizeName (const juce::String& name);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String currentPresetName;
    juce::ValueTree lastLoadedState;
};
