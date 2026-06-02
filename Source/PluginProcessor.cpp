#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    static float getParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float fallback = 0.0f)
    {
        if (auto* value = apvts.getRawParameterValue (id))
            return value->load();
        return fallback;
    }

    static bool getBoolParam (juce::AudioProcessorValueTreeState& apvts, const char* id, bool fallback = false)
    {
        if (auto* value = apvts.getRawParameterValue (id))
            return value->load() > 0.5f;
        return fallback;
    }

    static double onePoleCoeff (double frequency, double sampleRate)
    {
        return 1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * frequency / juce::jmax (sampleRate, 1.0));
    }

    static double smoothCoeffMs (double timeMs, double sampleRate)
    {
        return 1.0 - std::exp (-1000.0 / (juce::jmax (timeMs, 0.01) * juce::jmax (sampleRate, 1.0)));
    }
}

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
        juce::ParameterID { "master_bypass", 1 }, "Master Bypass", false,
        juce::AudioParameterBoolAttributes().withAutomatable (true).withMeta (true)));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "noisegt1_suppression", 1 }, "NoiseGT1 Suppression", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "noisegt1_bypass", 1 }, "NoiseGT1 Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_low", 1 }, "EQ 4K Low", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lowmid", 1 }, "EQ 4K Low Mid", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_presence", 1 }, "EQ 4K Presence", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_air", 1 }, "EQ 4K Air", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "eq4k_bypass", 1 }, "EQ 4K Bypass", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_input", 1 }, "76 Input", juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_attack", 1 }, "76 Attack", juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_release", 1 }, "76 Release", juce::NormalisableRange<float> (0.0f, 1.0f), 0.65f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_output", 1 }, "76 Output", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "comp76_bypass", 1 }, "76 Bypass", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_peak", 1 }, "2A Peak Reduction", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_gain", 1 }, "2A Gain", juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "comp2a_bypass", 1 }, "2A Bypass", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_drive", 1 }, "Tube Drive", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_tone", 1 }, "Tube Tone", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "tube_bypass", 1 }, "Tube Bypass", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "esser_freq", 1 }, "Esser Freq", juce::NormalisableRange<float> (3000.0f, 12000.0f), 7000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "esser_amount", 1 }, "Esser Amount", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "esser_bypass", 1 }, "Esser Bypass", true));

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
    currentSampleRate = sampleRate;
    noiseGT1.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    resetInternalStates();
}

void D7SChannelStripFullAudioProcessor::releaseResources()
{
    noiseGT1.reset();
    resetInternalStates();
}

void D7SChannelStripFullAudioProcessor::resetInternalStates()
{
    eqLp100.fill (0.0); eqLp500.fill (0.0); eqLp1500.fill (0.0); eqLp3000.fill (0.0); eqLp6000.fill (0.0);
    comp76Env.fill (0.0); comp2aEnv.fill (0.0);
    esserLp.fill (0.0); esserEnv.fill (0.0);
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

template <typename FloatType>
void D7SChannelStripFullAudioProcessor::processAudioBlock (juce::AudioBuffer<FloatType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalInputChannels  = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    if (getBoolParam (apvts, "master_bypass"))
        return;

    noiseGT1.setSuppressionAmount (getParam (apvts, "noisegt1_suppression", 0.5f));
    noiseGT1.setBypass (getBoolParam (apvts, "noisegt1_bypass"));
    noiseGT1.process (buffer);

    const int numCh = juce::jmin (buffer.getNumChannels(), (int) eqLp100.size());
    const int numSamples = buffer.getNumSamples();

    const double c100  = onePoleCoeff (100.0, currentSampleRate);
    const double c500  = onePoleCoeff (500.0, currentSampleRate);
    const double c1500 = onePoleCoeff (1500.0, currentSampleRate);
    const double c3000 = onePoleCoeff (3000.0, currentSampleRate);
    const double c6000 = onePoleCoeff (6000.0, currentSampleRate);

    const bool eqOn = ! getBoolParam (apvts, "eq4k_bypass", true);
    const double lowGain = juce::Decibels::decibelsToGain ((double) getParam (apvts, "eq4k_low"));
    const double lowMidGain = juce::Decibels::decibelsToGain ((double) getParam (apvts, "eq4k_lowmid"));
    const double presenceGain = juce::Decibels::decibelsToGain ((double) getParam (apvts, "eq4k_presence"));
    const double airGain = juce::Decibels::decibelsToGain ((double) getParam (apvts, "eq4k_air"));

    const bool comp76On = ! getBoolParam (apvts, "comp76_bypass", true);
    const double c76Input = juce::Decibels::decibelsToGain (-6.0 + getParam (apvts, "comp76_input") * 24.0);
    const double c76Attack = smoothCoeffMs (0.2 + (1.0 - getParam (apvts, "comp76_attack")) * 8.0, currentSampleRate);
    const double c76Release = smoothCoeffMs (30.0 + getParam (apvts, "comp76_release") * 450.0, currentSampleRate);
    const double c76Output = juce::Decibels::decibelsToGain ((double) getParam (apvts, "comp76_output"));

    const bool comp2aOn = ! getBoolParam (apvts, "comp2a_bypass", true);
    const double c2aPeak = getParam (apvts, "comp2a_peak");
    const double c2aGain = juce::Decibels::decibelsToGain ((double) getParam (apvts, "comp2a_gain"));
    const double c2aAttack = smoothCoeffMs (12.0, currentSampleRate);
    const double c2aRelease = smoothCoeffMs (650.0, currentSampleRate);

    const bool tubeOn = ! getBoolParam (apvts, "tube_bypass", true);
    const double tubeDrive = getParam (apvts, "tube_drive");
    const double tubeTone = getParam (apvts, "tube_tone");

    const bool esserOn = ! getBoolParam (apvts, "esser_bypass", true);
    const double esserFreq = getParam (apvts, "esser_freq", 7000.0f);
    const double esserAmount = getParam (apvts, "esser_amount");
    const double esserCoeff = onePoleCoeff (esserFreq, currentSampleRate);
    const double esserAtk = smoothCoeffMs (1.0, currentSampleRate);
    const double esserRel = smoothCoeffMs (90.0, currentSampleRate);

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            double x = (double) data[i];

            eqLp100[ch]  += c100  * (x - eqLp100[ch]);
            eqLp500[ch]  += c500  * (x - eqLp500[ch]);
            eqLp1500[ch] += c1500 * (x - eqLp1500[ch]);
            eqLp3000[ch] += c3000 * (x - eqLp3000[ch]);
            eqLp6000[ch] += c6000 * (x - eqLp6000[ch]);

            if (eqOn)
            {
                const double low = eqLp100[ch];
                const double lowMid = eqLp500[ch] - eqLp1500[ch];
                const double presence = eqLp3000[ch] - eqLp6000[ch];
                const double air = x - eqLp6000[ch];
                x += (low * (lowGain - 1.0))
                   + (lowMid * (lowMidGain - 1.0))
                   + (presence * (presenceGain - 1.0))
                   + (air * (airGain - 1.0));
                x = std::tanh (x * 1.02) / std::tanh (1.02);
            }

            if (comp76On)
            {
                x *= c76Input;
                const double det = std::abs (x);
                const double coeff = det > comp76Env[ch] ? c76Attack : c76Release;
                comp76Env[ch] += coeff * (det - comp76Env[ch]);
                const double envDb = juce::Decibels::gainToDecibels (juce::jmax (comp76Env[ch], 1e-9));
                const double over = envDb - (-24.0);
                const double grDb = over > 0.0 ? over * 0.75 : 0.0;
                x *= juce::Decibels::decibelsToGain (-grDb);
                x = std::tanh (x * 1.15) / std::tanh (1.15);
                x *= c76Output;
            }

            if (comp2aOn)
            {
                const double threshold = -8.0 - c2aPeak * 34.0;
                const double detDb = juce::Decibels::gainToDecibels (juce::jmax (std::abs (x), 1e-9));
                const double target = detDb > threshold ? (detDb - threshold) * 0.55 : 0.0;
                const double coeff = target > comp2aEnv[ch] ? c2aAttack : c2aRelease;
                comp2aEnv[ch] += coeff * (target - comp2aEnv[ch]);
                x *= juce::Decibels::decibelsToGain (-comp2aEnv[ch]);
                x = std::tanh (x * 1.06) / std::tanh (1.06);
                x *= c2aGain;
            }

            if (tubeOn)
            {
                const double drive = 1.0 + tubeDrive * 10.0;
                const double shaped = std::tanh (x * drive) / std::tanh (drive);
                const double darker = eqLp3000[ch];
                const double brighter = shaped;
                x = juce::jmap (tubeTone, darker, brighter);
                x *= juce::Decibels::decibelsToGain (-tubeDrive * 3.0);
            }

            if (esserOn)
            {
                esserLp[ch] += esserCoeff * (x - esserLp[ch]);
                const double high = x - esserLp[ch];
                const double level = std::abs (high);
                const double coeff = level > esserEnv[ch] ? esserAtk : esserRel;
                esserEnv[ch] += coeff * (level - esserEnv[ch]);
                const double levelDb = juce::Decibels::gainToDecibels (juce::jmax (esserEnv[ch], 1e-9));
                const double threshold = -38.0 + (1.0 - esserAmount) * 28.0;
                const double over = juce::jmax (0.0, levelDb - threshold);
                const double reduction = juce::jlimit (0.0, 0.85, (over / 24.0) * esserAmount);
                x -= high * reduction;
            }

            data[i] = (FloatType) juce::jlimit (-4.0, 4.0, x);
        }
    }
}

void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    processAudioBlock (buffer);
}

void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    processAudioBlock (buffer);
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
