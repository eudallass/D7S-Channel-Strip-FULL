#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    static float readAtomic (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }

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

    static double peakDbFromLinear (double value)
    {
        return juce::Decibels::gainToDecibels (juce::jmax (value, 1.0e-9));
    }

    static float logFrequencyForBin (int index, int totalBins)
    {
        const float minHz = 20.0f;
        const float maxHz = 20000.0f;
        const float normalised = (float) index / (float) juce::jmax (1, totalBins - 1);
        return minHz * std::pow (maxHz / minHz, normalised);
    }
}

D7SChannelStripFullAudioProcessor::D7SChannelStripFullAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "D7S_PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < numRackModules; ++i)
        rackModuleOrder[(size_t) i].store (i);
    cacheParameterPointers();
}

D7SChannelStripFullAudioProcessor::~D7SChannelStripFullAudioProcessor() {}

void D7SChannelStripFullAudioProcessor::cacheParameterPointers()
{
    masterBypassParam = apvts.getRawParameterValue ("master_bypass");
    rackInputParam = apvts.getRawParameterValue ("rack_input");
    rackOutputParam = apvts.getRawParameterValue ("rack_output");
    rackMixParam = apvts.getRawParameterValue ("rack_mix");
    noiseGT1.cacheParameters (apvts);
    eq4k.cacheParameters (apvts);
    comp76.cacheParameters (apvts);
    comp2a.cacheParameters (apvts);
    tube.cacheParameters (apvts);
    esser.cacheParameters (apvts);
}

juce::AudioProcessorParameter* D7SChannelStripFullAudioProcessor::getBypassParameter() const { return apvts.getParameter ("master_bypass"); }
const juce::String D7SChannelStripFullAudioProcessor::getName() const { return JucePlugin_Name; }
bool D7SChannelStripFullAudioProcessor::acceptsMidi() const { return false; }
bool D7SChannelStripFullAudioProcessor::producesMidi() const { return false; }
bool D7SChannelStripFullAudioProcessor::isMidiEffect() const { return false; }
double D7SChannelStripFullAudioProcessor::getTailLengthSeconds() const { return 2.5; }
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
    return (index < 0 || index >= numSpectrumBins) ? -120.0f : spectrumBinsDb[(size_t) index].load();
}

void D7SChannelStripFullAudioProcessor::pushSpectrumSample (float sample) noexcept
{
    spectrumFifo[(size_t) spectrumFifoIndex++] = sample;
    if (spectrumFifoIndex >= spectrumFFTSize)
    {
        runSpectrumFFT();
        spectrumFifoIndex = 0;
    }
}

void D7SChannelStripFullAudioProcessor::runSpectrumFFT() noexcept
{
    std::fill (spectrumFftData.begin(), spectrumFftData.end(), 0.0f);
    std::copy (spectrumFifo.begin(), spectrumFifo.end(), spectrumFftData.begin());
    spectrumWindow.multiplyWithWindowingTable (spectrumFftData.data(), spectrumFFTSize);
    spectrumFFT.performFrequencyOnlyForwardTransform (spectrumFftData.data(), true);

    const float nyquist = (float) currentSampleRate * 0.5f;
    const int fftBins = spectrumFFTSize / 2;
    for (int band = 0; band < numSpectrumBins; ++band)
    {
        const float f0 = logFrequencyForBin (band, numSpectrumBins);
        const float f1 = logFrequencyForBin (juce::jmin (band + 1, numSpectrumBins - 1), numSpectrumBins);
        const int bin0 = juce::jlimit (1, fftBins - 1, (int) std::floor ((f0 / nyquist) * (float) fftBins));
        const int bin1 = juce::jlimit (bin0 + 1, fftBins, (int) std::ceil ((f1 / nyquist) * (float) fftBins));
        float energy = 0.0f;
        int count = 0;
        for (int bin = bin0; bin < bin1; ++bin)
        {
            const float mag = spectrumFftData[(size_t) bin] / (float) spectrumFFTSize;
            energy += mag * mag;
            ++count;
        }
        const float rms = std::sqrt (energy / (float) juce::jmax (1, count));
        float targetDb = juce::Decibels::gainToDecibels (rms, -120.0f) + 42.0f;
        targetDb = juce::jlimit (-96.0f, 12.0f, targetDb);
        auto& smooth = spectrumSmoothedDb[(size_t) band];
        smooth += (targetDb > smooth ? 0.48f : 0.12f) * (targetDb - smooth);
        auto& peak = spectrumPeakDb[(size_t) band];
        if (smooth > peak) peak = smooth; else peak -= 0.55f;
        spectrumBinsDb[(size_t) band].store (juce::jmax (smooth, peak - 18.0f));
    }
}

void D7SChannelStripFullAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    noiseGT1.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    eq4k.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    comp76.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    comp2a.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    tube.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    esser.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    delayGlide.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    resetInternalStates();
}

void D7SChannelStripFullAudioProcessor::releaseResources()
{
    noiseGT1.reset(); eq4k.reset(); comp76.reset(); comp2a.reset(); tube.reset(); esser.reset(); delayGlide.reset();
    resetInternalStates();
}

void D7SChannelStripFullAudioProcessor::resetInternalStates()
{
    spectrumFifo.fill (0.0f);
    spectrumFftData.fill (0.0f);
    spectrumSmoothedDb.fill (-96.0f);
    spectrumPeakDb.fill (-96.0f);
    spectrumFifoIndex = 0;
    rackInputPeakDb.store (-120.0f);
    rackOutputPeakDb.store (-120.0f);
    for (auto& bin : spectrumBinsDb)
        bin.store (-96.0f);
}

bool D7SChannelStripFullAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

template <typename FloatType>
void D7SChannelStripFullAudioProcessor::processModuleById (int moduleId, juce::AudioBuffer<FloatType>& buffer, double bpm)
{
    switch (moduleId)
    {
        case moduleNoiseGT1: noiseGT1.process (buffer); break;
        case moduleEQ4K:     eq4k.process (buffer); break;
        case module76:       comp76.process (buffer); break;
        case module2A:       comp2a.process (buffer); break;
        case moduleTube:     tube.process (buffer); break;
        case moduleEsser:    esser.process (buffer); break;
        case moduleDelay:
            delayGlide.setTempoBpm (bpm);
            delayGlide.setMix (getParam (apvts, "delay_mix", 25.0f));
            delayGlide.setFeedback (getParam (apvts, "delay_feedback", 35.0f));
            delayGlide.setDelayDivision (getChoiceParam (apvts, "delay_time", 3));
            delayGlide.setBypass (getBoolParam (apvts, "delay_bypass", true));
            delayGlide.setGlideEnabled (getBoolParam (apvts, "delay_glide_on", false));
            delayGlide.setGlideDirection (getChoiceParam (apvts, "delay_glide_direction", 0));
            delayGlide.setGlideTime (getParam (apvts, "delay_glide_time", 35.0f));
            delayGlide.process (buffer);
            break;
        default: break;
    }
}

template <typename FloatType>
void D7SChannelStripFullAudioProcessor::processAudioBlock (juce::AudioBuffer<FloatType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
    if (readAtomic (masterBypassParam, 0.0f) > 0.5f) return;

    const int numCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    const double rackIn = juce::Decibels::decibelsToGain ((double) readAtomic (rackInputParam, 0.0f));
    const double rackOut = juce::Decibels::decibelsToGain ((double) readAtomic (rackOutputParam, 0.0f));
    const double rackMix = readAtomic (rackMixParam, 100.0f) / 100.0;
    double inputPeak = 0.0, outputPeak = 0.0;
    juce::AudioBuffer<FloatType> rackDry (numCh, n);

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto* dry = rackDry.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            data[i] = (FloatType) ((double) data[i] * rackIn);
            dry[i] = data[i];
            inputPeak = juce::jmax (inputPeak, std::abs ((double) data[i]));
        }
    }

    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (playHead->getCurrentPosition (info) && info.bpm > 0.0) bpm = info.bpm;
    }

    const auto order = getModuleOrder();
    for (int slot = 0; slot < numRackModules; ++slot)
        processModuleById (order[(size_t) slot], buffer, bpm);

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto* dry = rackDry.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            double x = (double) dry[i] * (1.0 - rackMix) + (double) data[i] * rackMix;
            x *= rackOut;
            outputPeak = juce::jmax (outputPeak, std::abs (x));
            data[i] = (FloatType) juce::jlimit (-4.0, 4.0, x);
            if (ch == 0) pushSpectrumSample ((float) x);
        }
    }
    rackInputPeakDb.store ((float) peakDbFromLinear (inputPeak));
    rackOutputPeakDb.store ((float) peakDbFromLinear (outputPeak));
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
