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
        false,
        juce::AudioParameterBoolAttributes()
            .withAutomatable (true)
            .withMeta (true)
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "noisegt1_suppression", 1 },
        "NoiseGT1 Suppression",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.5f
    ));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "noisegt1_bypass", 1 },
        "NoiseGT1 Bypass",
        false
    ));

    return { params.begin(), params.end() };
}

juce::AudioProcessorParameter* D7SChannelStripFullAudioProcessor::getBypassParameter() const
{
    return apvts.getParameter ("master_bypass");
}

const juce::String D7SChannelStripFullAudioProcessor::getName() const { return JucePlugin_Name; }
bool D7SChannelStripFullAudioProcessor::acceptsMidi() const { return false; }
bool D7SChannelStripFullAudioProcessor::producesMidi() const { return false; }
bool D7SChannelStripFullAudioProcessor::isMidiEffect() const { return false; }
double D7SChannelStripFullAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int D7SChannelStripFullAudioProcessor::getNumPrograms() { return 1; }
int D7SChannelStripFullAudioProcessor::getCurrentProgram() { return 0; }
void D7SChannelStripFullAudioProcessor::setCurrentProgram (int) {}
const juce::String D7SChannelStripFullAudioProcessor::getProgramName (int) { return {}; }
void D7SChannelStripFullAudioProcessor::changeProgramName (int, const juce::String&) {}

void D7SChannelStripFullAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    noiseGT1.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}
void D7SChannelStripFullAudioProcessor::releaseResources()
{
    noiseGT1.reset();
}

bool D7SChannelStripFullAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    const bool inDisabled  = (in  == juce::AudioChannelSet::disabled());
    const bool outDisabled = (out == juce::AudioChannelSet::disabled());

    if (inDisabled || outDisabled)
        return true;

    if (in != out)
        return false;

    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalInputChannels  = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    auto* masterBypass = apvts.getRawParameterValue ("master_bypass");
    if (masterBypass != nullptr && masterBypass->load() > 0.5f)
        return;

    // NoiseGT1
    auto* suppression = apvts.getRawParameterValue ("noisegt1_suppression");
    auto* noiseBypass = apvts.getRawParameterValue ("noisegt1_bypass");
    if (suppression != nullptr) noiseGT1.setSuppressionAmount (suppression->load());
    if (noiseBypass != nullptr) noiseGT1.setBypass (noiseBypass->load() > 0.5f);
    noiseGT1.process (buffer);
}

void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalInputChannels  = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    auto* masterBypass = apvts.getRawParameterValue ("master_bypass");
    if (masterBypass != nullptr && masterBypass->load() > 0.5f)
        return;

    // NoiseGT1
    auto* suppression = apvts.getRawParameterValue ("noisegt1_suppression");
    auto* noiseBypass = apvts.getRawParameterValue ("noisegt1_bypass");
    if (suppression != nullptr) noiseGT1.setSuppressionAmount (suppression->load());
    if (noiseBypass != nullptr) noiseGT1.setBypass (noiseBypass->load() > 0.5f);
    noiseGT1.process (buffer);
}

bool D7SChannelStripFullAudioProcessor::hasEditor() const { return true; }
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
    {
        auto state = juce::ValueTree::fromXml (*xmlState);
        if (state.isValid())
            apvts.replaceState (state);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new D7SChannelStripFullAudioProcessor();
}
