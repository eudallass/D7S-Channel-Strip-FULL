#include "PluginProcessor.h"
#include "PluginEditor.h"

D7SChannelStripFullAudioProcessor::D7SChannelStripFullAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "D7S_PARAMETERS", createParameterLayout())
{
}

D7SChannelStripFullAudioProcessor::~D7SChannelStripFullAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout D7SChannelStripFullAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "master_bypass", 1 },
        "Master Bypass",
        false
    ));

    return { params.begin(), params.end() };
}

const juce::String D7SChannelStripFullAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool D7SChannelStripFullAudioProcessor::acceptsMidi() const { return false; }
bool D7SChannelStripFullAudioProcessor::producesMidi() const { return false; }
bool D7SChannelStripFullAudioProcessor::isMidiEffect() const { return false; }
double D7SChannelStripFullAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int D7SChannelStripFullAudioProcessor::getNumPrograms() { return 1; }
int D7SChannelStripFullAudioProcessor::getCurrentProgram() { return 0; }
void D7SChannelStripFullAudioProcessor::setCurrentProgram (int) {}
const juce::String D7SChannelStripFullAudioProcessor::getProgramName (int) { return {}; }
void D7SChannelStripFullAudioProcessor::changeProgramName (int, const juce::String&) {}

void D7SChannelStripFullAudioProcessor::prepareToPlay (double, int)
{
}

void D7SChannelStripFullAudioProcessor::releaseResources()
{
}

bool D7SChannelStripFullAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainInput  = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput == juce::AudioChannelSet::disabled()
        && mainOutput == juce::AudioChannelSet::disabled())
        return true;

    if (mainOutput != juce::AudioChannelSet::mono()
        && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    if (mainInput != mainOutput)
        return false;

    return true;
}

void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

bool D7SChannelStripFullAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* D7SChannelStripFullAudioProcessor::createEditor()
{
    return new D7SChannelStripFullAudioProcessorEditor (*this);
}

void D7SChannelStripFullAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());

    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void D7SChannelStripFullAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new D7SChannelStripFullAudioProcessor();
}
