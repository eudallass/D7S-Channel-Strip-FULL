#include "PluginProcessor.h"
#include "PluginEditor.h"

D7SChannelStripFullAudioProcessor::D7SChannelStripFullAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), false)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), false))
{
}

D7SChannelStripFullAudioProcessor::~D7SChannelStripFullAudioProcessor()
{
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

void D7SChannelStripFullAudioProcessor::prepareToPlay (double, int) {}
void D7SChannelStripFullAudioProcessor::releaseResources() {}

bool D7SChannelStripFullAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto input  = layouts.getMainInputChannelSet();
    auto output = layouts.getMainOutputChannelSet();

    if (input == juce::AudioChannelSet::disabled()
        && output == juce::AudioChannelSet::disabled())
        return true;

    if (output == juce::AudioChannelSet::disabled())
        return input == juce::AudioChannelSet::disabled();

    if (input == juce::AudioChannelSet::disabled())
        return output == juce::AudioChannelSet::mono()
            || output == juce::AudioChannelSet::stereo();

    if (input != output)
        return false;

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

bool D7SChannelStripFullAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* D7SChannelStripFullAudioProcessor::createEditor()
{
    return new D7SChannelStripFullAudioProcessorEditor (*this);
}

void D7SChannelStripFullAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void D7SChannelStripFullAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new D7SChannelStripFullAudioProcessor();
}
