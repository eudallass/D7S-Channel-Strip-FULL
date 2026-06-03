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

    static int getChoiceParam (juce::AudioProcessorValueTreeState& apvts, const char* id, int fallback = 0)
    {
        if (auto* value = apvts.getRawParameterValue (id))
            return (int) std::round (value->load());
        return fallback;
    }

    static double onePoleCoeff (double frequency, double sampleRate)
    {
        return 1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * juce::jlimit (10.0, sampleRate * 0.45, frequency) / juce::jmax (sampleRate, 1.0));
    }

    static double smoothCoeffMs (double timeMs, double sampleRate)
    {
        return 1.0 - std::exp (-1000.0 / (juce::jmax (timeMs, 0.01) * juce::jmax (sampleRate, 1.0)));
    }

    static double peakDbFromLinear (double value)
    {
        return juce::Decibels::gainToDecibels (juce::jmax (value, 1.0e-9));
    }

    static double spectrumFrequencyForIndex (int index)
    {
        const double minHz = 25.0;
        const double maxHz = 21000.0;
        const double normalised = (double) index / (double) D7SChannelStripFullAudioProcessor::numSpectrumBins;
        return minHz * std::pow (maxHz / minHz, normalised);
    }
}

D7SChannelStripFullAudioProcessor::D7SChannelStripFullAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "D7S_PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < numRackModules; ++i)
        rackModuleOrder[(size_t) i].store (i);
}

D7SChannelStripFullAudioProcessor::~D7SChannelStripFullAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout D7SChannelStripFullAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "master_bypass", 1 }, "Master Bypass", false, juce::AudioParameterBoolAttributes().withAutomatable (true).withMeta (true)));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "rack_input", 1 }, "Rack Input", juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "rack_output", 1 }, "Rack Output", juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "rack_mix", 1 }, "Rack Mix", juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "noisegt1_suppression", 1 }, "NoiseGT1 Suppression", juce::NormalisableRange<float> (0.0f, 100.0f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "noisegt1_bypass", 1 }, "NoiseGT1 Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hpf", 1 }, "EQ 4K HPF", juce::NormalisableRange<float> (20.0f, 500.0f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lpf", 1 }, "EQ 4K LPF", juce::NormalisableRange<float> (4000.0f, 22000.0f), 22000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lf_freq", 1 }, "EQ 4K LF Freq", juce::NormalisableRange<float> (30.0f, 450.0f), 80.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lf_gain", 1 }, "EQ 4K LF Gain", juce::NormalisableRange<float> (-15.0f, 15.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "eq4k_lf_bell", 1 }, "EQ 4K LF Bell", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lmf_freq", 1 }, "EQ 4K LMF Freq", juce::NormalisableRange<float> (200.0f, 2500.0f), 600.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lmf_gain", 1 }, "EQ 4K LMF Gain", juce::NormalisableRange<float> (-15.0f, 15.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_lmf_q", 1 }, "EQ 4K LMF Q", juce::NormalisableRange<float> (0.4f, 4.0f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hmf_freq", 1 }, "EQ 4K HMF Freq", juce::NormalisableRange<float> (600.0f, 7000.0f), 3000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hmf_gain", 1 }, "EQ 4K HMF Gain", juce::NormalisableRange<float> (-15.0f, 15.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hmf_q", 1 }, "EQ 4K HMF Q", juce::NormalisableRange<float> (0.4f, 4.0f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hf_freq", 1 }, "EQ 4K HF Freq", juce::NormalisableRange<float> (1500.0f, 16000.0f), 10000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "eq4k_hf_gain", 1 }, "EQ 4K HF Gain", juce::NormalisableRange<float> (-15.0f, 15.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "eq4k_hf_bell", 1 }, "EQ 4K HF Bell", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "eq4k_bypass", 1 }, "EQ 4K Bypass", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_input", 1 }, "76 Input", juce::NormalisableRange<float> (0.0f, 10.0f), 4.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_output", 1 }, "76 Output", juce::NormalisableRange<float> (0.0f, 10.0f), 5.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_attack", 1 }, "76 Attack", juce::NormalisableRange<float> (1.0f, 7.0f), 3.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp76_release", 1 }, "76 Release", juce::NormalisableRange<float> (1.0f, 7.0f), 5.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "comp76_ratio", 1 }, "76 Ratio", juce::StringArray { "4", "8", "12", "20", "All" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "comp76_bypass", 1 }, "76 Bypass", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_peak", 1 }, "2A Peak Reduction", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_gain", 1 }, "2A Gain", juce::NormalisableRange<float> (0.0f, 100.0f), 40.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "comp2a_mode", 1 }, "2A Mode", juce::StringArray { "Compress", "Limit" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_emphasis", 1 }, "2A HF Emphasis", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "comp2a_mix", 1 }, "2A Mix", juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "comp2a_bypass", 1 }, "2A Bypass", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_beauty", 1 }, "Tube Beauty", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_beast", 1 }, "Tube Beast", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_sensitivity", 1 }, "Tube Sensitivity", juce::NormalisableRange<float> (0.0f, 100.0f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tube_mix", 1 }, "Tube Mix", juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "tube_bypass", 1 }, "Tube Bypass", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "esser_threshold", 1 }, "Esser Threshold", juce::NormalisableRange<float> (-60.0f, 0.0f), -24.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "esser_freq", 1 }, "Esser Frequency", juce::NormalisableRange<float> (500.0f, 20000.0f), 7000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "esser_range", 1 }, "Esser Range", juce::NormalisableRange<float> (0.0f, 24.0f), 12.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "esser_mode", 1 }, "Esser Mode", juce::StringArray { "Wide", "Split" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "esser_bypass", 1 }, "Esser Bypass", true));

    return { params.begin(), params.end() };
}

juce::AudioProcessorParameter* D7SChannelStripFullAudioProcessor::getBypassParameter() const { return apvts.getParameter ("master_bypass"); }
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

void D7SChannelStripFullAudioProcessor::setModuleOrder (const std::array<int, numRackModules>& newOrder) noexcept
{
    for (int i = 0; i < numRackModules; ++i)
        rackModuleOrder[(size_t) i].store (juce::jlimit (0, numRackModules - 1, newOrder[(size_t) i]));
}

std::array<int, D7SChannelStripFullAudioProcessor::numRackModules> D7SChannelStripFullAudioProcessor::getModuleOrder() const noexcept
{
    std::array<int, numRackModules> result {};
    for (int i = 0; i < numRackModules; ++i)
        result[(size_t) i] = rackModuleOrder[(size_t) i].load();
    return result;
}

float D7SChannelStripFullAudioProcessor::getSpectrumBinDb (int index) const noexcept
{
    if (index < 0 || index >= numSpectrumBins)
        return -120.0f;
    return spectrumBinsDb[(size_t) index].load();
}

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
    eqHpfState.fill (0.0); eqLpfState.fill (0.0); eqLfState.fill (0.0);
    eqLmfLow.fill (0.0); eqLmfHigh.fill (0.0); eqHmfLow.fill (0.0); eqHmfHigh.fill (0.0); eqHfState.fill (0.0);
    comp76Env.fill (0.0); comp2aEnv.fill (0.0); tubeBeautyState.fill (0.0); tubeBeastState.fill (0.0);
    esserLp.fill (0.0); esserEnv.fill (0.0);
    spectrumLp.fill (0.0); spectrumEnergy.fill (0.0);
    rackInputPeakDb.store (-120.0f); rackOutputPeakDb.store (-120.0f);
    comp76GainReductionDb.store (0.0f); comp2aGainReductionDb.store (0.0f); esserGainReductionDb.store (0.0f);
    for (auto& bin : spectrumBinsDb)
        bin.store (-120.0f);
}

bool D7SChannelStripFullAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in == juce::AudioChannelSet::disabled() || out == juce::AudioChannelSet::disabled()) return true;
    if (in != out) return false;
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

template <typename FloatType>
void D7SChannelStripFullAudioProcessor::processAudioBlock (juce::AudioBuffer<FloatType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalInputChannels  = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();
    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    if (getBoolParam (apvts, "master_bypass")) return;

    const int numCh = juce::jmin (buffer.getNumChannels(), (int) eqHpfState.size());
    const int numSamples = buffer.getNumSamples();
    const double rackIn = juce::Decibels::decibelsToGain ((double) getParam (apvts, "rack_input"));
    const double rackOut = juce::Decibels::decibelsToGain ((double) getParam (apvts, "rack_output"));
    const double rackMix = getParam (apvts, "rack_mix", 100.0f) / 100.0;

    double inputPeak = 0.0;
    double outputPeak = 0.0;
    double max76Gr = 0.0, max2aGr = 0.0, maxEsserGr = 0.0;

    juce::AudioBuffer<FloatType> rackDry (buffer.getNumChannels(), numSamples);

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto* dry = rackDry.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            data[i] = (FloatType) ((double) data[i] * rackIn);
            dry[i] = data[i];
            inputPeak = juce::jmax (inputPeak, std::abs ((double) data[i]));
        }
    }

    const auto order = getModuleOrder();

    auto processNoise = [&]
    {
        noiseGT1.setSuppressionAmount (getParam (apvts, "noisegt1_suppression", 50.0f) / 100.0f);
        noiseGT1.setBypass (getBoolParam (apvts, "noisegt1_bypass"));
        noiseGT1.process (buffer);
    };

    auto processEQ = [&]
    {
        if (getBoolParam (apvts, "eq4k_bypass", true)) return;
        const double hpfCoeff = onePoleCoeff (getParam (apvts, "eq4k_hpf", 20.0f), currentSampleRate);
        const double lpfCoeff = onePoleCoeff (getParam (apvts, "eq4k_lpf", 22000.0f), currentSampleRate);
        const double lfCoeff = onePoleCoeff (getParam (apvts, "eq4k_lf_freq", 80.0f), currentSampleRate);
        const double lmfFreq = getParam (apvts, "eq4k_lmf_freq", 600.0f);
        const double lmfQ = getParam (apvts, "eq4k_lmf_q", 1.0f);
        const double hmfFreq = getParam (apvts, "eq4k_hmf_freq", 3000.0f);
        const double hmfQ = getParam (apvts, "eq4k_hmf_q", 1.0f);
        const double hfCoeff = onePoleCoeff (getParam (apvts, "eq4k_hf_freq", 10000.0f), currentSampleRate);
        const double lfGain = juce::Decibels::decibelsToGain ((double) getParam (apvts, "eq4k_lf_gain"));
        const double lmfGain = juce::Decibels::decibelsToGain ((double) getParam (apvts, "eq4k_lmf_gain"));
        const double hmfGain = juce::Decibels::decibelsToGain ((double) getParam (apvts, "eq4k_hmf_gain"));
        const double hfGain = juce::Decibels::decibelsToGain ((double) getParam (apvts, "eq4k_hf_gain"));
        const bool lfBell = getBoolParam (apvts, "eq4k_lf_bell");
        const bool hfBell = getBoolParam (apvts, "eq4k_hf_bell");
        const double lmfLowCoeff = onePoleCoeff (lmfFreq / juce::jmax (lmfQ, 0.4), currentSampleRate);
        const double lmfHighCoeff = onePoleCoeff (lmfFreq * juce::jmax (lmfQ, 0.4), currentSampleRate);
        const double hmfLowCoeff = onePoleCoeff (hmfFreq / juce::jmax (hmfQ, 0.4), currentSampleRate);
        const double hmfHighCoeff = onePoleCoeff (hmfFreq * juce::jmax (hmfQ, 0.4), currentSampleRate);

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                double x = (double) data[i];
                eqHpfState[ch] += hpfCoeff * (x - eqHpfState[ch]); x -= eqHpfState[ch];
                eqLpfState[ch] += lpfCoeff * (x - eqLpfState[ch]); x = eqLpfState[ch];
                eqLfState[ch] += lfCoeff * (x - eqLfState[ch]);
                eqLmfLow[ch] += lmfLowCoeff * (x - eqLmfLow[ch]); eqLmfHigh[ch] += lmfHighCoeff * (x - eqLmfHigh[ch]);
                eqHmfLow[ch] += hmfLowCoeff * (x - eqHmfLow[ch]); eqHmfHigh[ch] += hmfHighCoeff * (x - eqHmfHigh[ch]);
                eqHfState[ch] += hfCoeff * (x - eqHfState[ch]);
                const double lfBand = lfBell ? (eqLfState[ch] - eqLmfLow[ch]) : eqLfState[ch];
                const double lmfBand = eqLmfHigh[ch] - eqLmfLow[ch];
                const double hmfBand = eqHmfHigh[ch] - eqHmfLow[ch];
                const double hfBand = hfBell ? (eqHfState[ch] - eqHmfHigh[ch]) : (x - eqHfState[ch]);
                x += lfBand * (lfGain - 1.0) + lmfBand * (lmfGain - 1.0) + hmfBand * (hmfGain - 1.0) + hfBand * (hfGain - 1.0);
                data[i] = (FloatType) (std::tanh (x * 1.015) / std::tanh (1.015));
            }
        }
    };

    auto process76 = [&]
    {
        if (getBoolParam (apvts, "comp76_bypass", true)) return;
        const double c76Input = juce::Decibels::decibelsToGain (-18.0 + getParam (apvts, "comp76_input") * 4.2);
        const double c76Output = juce::Decibels::decibelsToGain (-18.0 + getParam (apvts, "comp76_output") * 3.6);
        const double c76Attack = smoothCoeffMs (0.02 + (7.0 - getParam (apvts, "comp76_attack", 3.0f)) * 1.15, currentSampleRate);
        const double c76Release = smoothCoeffMs (40.0 + (7.0 - getParam (apvts, "comp76_release", 5.0f)) * 180.0, currentSampleRate);
        const int ratioIndex = getChoiceParam (apvts, "comp76_ratio", 0);
        const double ratio = ratioIndex == 0 ? 4.0 : ratioIndex == 1 ? 8.0 : ratioIndex == 2 ? 12.0 : ratioIndex == 3 ? 20.0 : 30.0;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                double x = (double) data[i] * c76Input;
                const double det = std::abs (x);
                const double coeff = det > comp76Env[ch] ? c76Attack : c76Release;
                comp76Env[ch] += coeff * (det - comp76Env[ch]);
                const double over = peakDbFromLinear (comp76Env[ch]) - (-24.0);
                const double grDb = over > 0.0 ? over * (1.0 - (1.0 / ratio)) : 0.0;
                max76Gr = juce::jmax (max76Gr, grDb);
                x *= juce::Decibels::decibelsToGain (-grDb);
                x = std::tanh (x * (ratioIndex == 4 ? 1.45 : 1.15)) / std::tanh (ratioIndex == 4 ? 1.45 : 1.15);
                data[i] = (FloatType) (x * c76Output);
            }
        }
    };

    auto process2A = [&]
    {
        if (getBoolParam (apvts, "comp2a_bypass", true)) return;
        const double c2aPeak = getParam (apvts, "comp2a_peak") / 100.0;
        const double c2aGain = juce::Decibels::decibelsToGain (-12.0 + getParam (apvts, "comp2a_gain") * 0.36);
        const bool c2aLimit = getChoiceParam (apvts, "comp2a_mode", 0) == 1;
        const double c2aEmphasis = getParam (apvts, "comp2a_emphasis") / 100.0;
        const double c2aMix = getParam (apvts, "comp2a_mix", 100.0f) / 100.0;
        const double c2aAttack = smoothCoeffMs (10.0, currentSampleRate);
        const double c2aRelease = smoothCoeffMs (700.0, currentSampleRate);
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const double dry2a = (double) data[i];
                const double detDb = peakDbFromLinear (std::abs (dry2a) * (1.0 + c2aEmphasis * 1.5));
                const double threshold = -8.0 - c2aPeak * 36.0;
                const double slope = c2aLimit ? 0.78 : 0.52;
                const double target = detDb > threshold ? (detDb - threshold) * slope : 0.0;
                const double coeff = target > comp2aEnv[ch] ? c2aAttack : c2aRelease;
                comp2aEnv[ch] += coeff * (target - comp2aEnv[ch]);
                max2aGr = juce::jmax (max2aGr, comp2aEnv[ch]);
                double wet = dry2a * juce::Decibels::decibelsToGain (-comp2aEnv[ch]);
                wet = std::tanh (wet * 1.05) / std::tanh (1.05);
                wet *= c2aGain;
                data[i] = (FloatType) (dry2a * (1.0 - c2aMix) + wet * c2aMix);
            }
        }
    };

    auto processTube = [&]
    {
        if (getBoolParam (apvts, "tube_bypass", true)) return;
        const double beauty = getParam (apvts, "tube_beauty") / 100.0;
        const double beast = getParam (apvts, "tube_beast") / 100.0;
        const double sens = juce::Decibels::decibelsToGain (-12.0 + getParam (apvts, "tube_sensitivity") * 0.24);
        const double tubeMix = getParam (apvts, "tube_mix", 100.0f) / 100.0;
        const double beautyCoeff = onePoleCoeff (2500.0, currentSampleRate);
        const double beastCoeff = onePoleCoeff (220.0, currentSampleRate);
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const double dryTube = (double) data[i];
                const double driven = dryTube * sens;
                tubeBeautyState[ch] += beautyCoeff * (driven - tubeBeautyState[ch]);
                tubeBeastState[ch] += beastCoeff * (driven - tubeBeastState[ch]);
                const double beautyWet = std::tanh ((driven - tubeBeautyState[ch]) * (1.0 + beauty * 5.0));
                const double beastWet = std::tanh (tubeBeastState[ch] * (1.0 + beast * 12.0));
                double wetTube = driven + beautyWet * beauty * 0.45 + beastWet * beast * 0.65;
                wetTube *= juce::Decibels::decibelsToGain (-(beauty * 1.5 + beast * 3.0));
                data[i] = (FloatType) (dryTube * (1.0 - tubeMix) + wetTube * tubeMix);
            }
        }
    };

    auto processEsser = [&]
    {
        if (getBoolParam (apvts, "esser_bypass", true)) return;
        const double esserFreq = getParam (apvts, "esser_freq", 7000.0f);
        const double esserThreshold = getParam (apvts, "esser_threshold", -24.0f);
        const double esserRange = getParam (apvts, "esser_range", 12.0f);
        const bool esserSplit = getChoiceParam (apvts, "esser_mode", 1) == 1;
        const double esserCoeff = onePoleCoeff (esserFreq, currentSampleRate);
        const double esserAtk = smoothCoeffMs (1.0, currentSampleRate);
        const double esserRel = smoothCoeffMs (90.0, currentSampleRate);
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                double x = (double) data[i];
                esserLp[ch] += esserCoeff * (x - esserLp[ch]);
                const double sib = x - esserLp[ch];
                const double level = std::abs (sib);
                const double coeff = level > esserEnv[ch] ? esserAtk : esserRel;
                esserEnv[ch] += coeff * (level - esserEnv[ch]);
                const double over = peakDbFromLinear (esserEnv[ch]) - esserThreshold;
                const double reductionDb = juce::jlimit (0.0, esserRange, over > 0.0 ? over * 0.75 : 0.0);
                maxEsserGr = juce::jmax (maxEsserGr, reductionDb);
                const double gr = juce::Decibels::decibelsToGain (-reductionDb);
                data[i] = (FloatType) (esserSplit ? (x - sib + sib * gr) : (x * gr));
            }
        }
    };

    for (int slot = 0; slot < numRackModules; ++slot)
    {
        switch (order[(size_t) slot])
        {
            case moduleNoiseGT1: processNoise(); break;
            case moduleEQ4K:     processEQ();    break;
            case module76:       process76();    break;
            case module2A:       process2A();    break;
            case moduleTube:     processTube();  break;
            case moduleEsser:    processEsser(); break;
            default: break;
        }
    }

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto* dry = rackDry.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            double x = (double) dry[i] * (1.0 - rackMix) + (double) data[i] * rackMix;
            x *= rackOut;
            outputPeak = juce::jmax (outputPeak, std::abs (x));

            double previousLow = 0.0;
            for (int bin = 0; bin < numSpectrumBins; ++bin)
            {
                const double coeff = onePoleCoeff (spectrumFrequencyForIndex (bin + 1), currentSampleRate);
                spectrumLp[(size_t) bin] += coeff * (x - spectrumLp[(size_t) bin]);
                const double band = spectrumLp[(size_t) bin] - previousLow;
                previousLow = spectrumLp[(size_t) bin];
                spectrumEnergy[(size_t) bin] += 0.006 * ((band * band) - spectrumEnergy[(size_t) bin]);
            }

            data[i] = (FloatType) juce::jlimit (-4.0, 4.0, x);
        }
    }

    rackInputPeakDb.store ((float) peakDbFromLinear (inputPeak));
    rackOutputPeakDb.store ((float) peakDbFromLinear (outputPeak));
    comp76GainReductionDb.store ((float) max76Gr);
    comp2aGainReductionDb.store ((float) max2aGr);
    esserGainReductionDb.store ((float) maxEsserGr);

    for (int bin = 0; bin < numSpectrumBins; ++bin)
        spectrumBinsDb[(size_t) bin].store ((float) peakDbFromLinear (std::sqrt (juce::jmax (spectrumEnergy[(size_t) bin], 1.0e-12))));
}

void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) { processAudioBlock (buffer); }
void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&) { processAudioBlock (buffer); }
bool D7SChannelStripFullAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* D7SChannelStripFullAudioProcessor::createEditor() { return new D7SChannelStripFullAudioProcessorEditor (*this); }

void D7SChannelStripFullAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr) copyXmlToBinary (*xml, destData);
}

void D7SChannelStripFullAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        auto state = juce::ValueTree::fromXml (*xmlState);
        if (state.isValid()) apvts.replaceState (state);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new D7SChannelStripFullAudioProcessor();
}
