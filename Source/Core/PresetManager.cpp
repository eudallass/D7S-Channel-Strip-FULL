#include "PresetManager.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
    installFactoryPresets();
    lastLoadedState = apvts.copyState();
}

juce::File PresetManager::getPresetsDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
        .getChildFile ("Library/Audio/Presets/D7S/Channel Strip FULL");
}

juce::File PresetManager::getFactoryDirectory() const
{
    return getPresetsDirectory().getChildFile ("Factory");
}

juce::String PresetManager::sanitizeName (const juce::String& name)
{
    auto safe = name.trim();
    safe = safe.replaceCharacters ("/:*?\"<>|", "---------");
    return safe.isEmpty() ? "Untitled" : safe;
}

juce::File PresetManager::presetFileForName (const juce::String& name) const
{
    return getPresetsDirectory().getChildFile (sanitizeName (name) + ".d7spreset");
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    auto dir = getPresetsDirectory();
    if (! dir.exists()) return names;
    juce::Array<juce::File> files;
    dir.findChildFiles (files, juce::File::findFiles, true, "*.d7spreset");
    for (auto& file : files)
        names.add (file.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool PresetManager::savePreset (const juce::String& name)
{
    auto dir = getPresetsDirectory();
    if (! dir.createDirectory()) return false;
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr) return false;
    auto file = presetFileForName (name);
    if (! xml->writeTo (file)) return false;
    currentPresetName = sanitizeName (name);
    lastLoadedState = state;
    return true;
}

bool PresetManager::loadPreset (const juce::String& name)
{
    auto file = presetFileForName (name);
    if (! file.existsAsFile())
        file = getFactoryDirectory().getChildFile (sanitizeName (name) + ".d7spreset");
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr) return false;
    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid()) return false;
    apvts.replaceState (state);
    currentPresetName = sanitizeName (name);
    lastLoadedState = state;
    return true;
}

bool PresetManager::deletePreset (const juce::String&)
{
    return false;
}

bool PresetManager::renamePreset (const juce::String&, const juce::String&)
{
    return false;
}

void PresetManager::installFactoryPresets()
{
    getFactoryDirectory().createDirectory();
}

bool PresetManager::isPresetDirty() const
{
    return lastLoadedState.isValid() && ! apvts.copyState().isEquivalentTo (lastLoadedState);
}
