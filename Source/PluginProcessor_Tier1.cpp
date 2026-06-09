#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    static float readAtomic (std::atomic<float>* p, float fallback) noexcept { return p != nullptr ? p->load() : fallback; }
    static float getParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float fallback = 0.0f) { if (auto* value = apvts.getRawParameterValue (id)) return value->load(); return fallback; }
    static bool getBoolParam (juce::AudioProcessorValueTreeState& apvts, const char* id, bool fallback = false) { if (auto* value = apvts.getRawParameterValue (id)) return value->load() > 0.5f; return fallback; }
    static int getChoiceParam (juce::AudioProcessorValueTreeState& apvts, const char* id, int fallback = 0) { if (auto* value = apvts.getRawParameterValue (id)) return (int) std::round (value->load()); return fallback; }
    static double peakDbFromLinear (double value) { return juce::Decibels::gainToDecibels (juce::jmax (value, 1.0e-9)); }
    static float logFrequencyForBin (int index, int totalBins) { const float minHz = 20.0f; const float maxHz = 20000.0f; const float normalised = (float) index / (float) juce::jmax (1, totalBins - 1); return minHz * std::pow (maxHz / minHz, normalised); }
    static float analyzerReleaseCoeffForSpeed (int speedIndex) noexcept { switch (speedIndex) { case 0: return 0.40f; case 1: return 0.20f; case 2: return 0.10f; case 3: return 0.04f; case 4: return 0.015f; default: return 0.04f; } }
    static void storeMeterWithDecay (std::atomic<float>& meter, float target) noexcept { const float current = meter.load(); const float smoothed = target > current ? target : current * 0.99f + target * 0.01f; meter.store (smoothed); }
}

D7SChannelStripFullAudioProcessor::D7SChannelStripFullAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread ("D7S Analyzer Thread"),
      apvts (*this, nullptr, "D7S_PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < numRackModules; ++i)
        rackModuleOrder[(size_t) i].store (i);
    cacheParameterPointers();
}

D7SChannelStripFullAudioProcessor::~D7SChannelStripFullAudioProcessor()
{
    stopThread (1000);
}

void D7SChannelStripFullAudioProcessor::cacheParameterPointers()
{
    masterBypassParam = apvts.getRawParameterValue ("master_bypass");
    rackInputParam = apvts.getRawParameterValue ("rack_input");
    rackOutputParam = apvts.getRawParameterValue ("rack_output");
    rackMixParam = apvts.getRawParameterValue ("rack_mix");
    analyzerPreEnabledParam = apvts.getRawParameterValue ("analyzer_pre_enabled");
    analyzerPostEnabledParam = apvts.getRawParameterValue ("analyzer_post_enabled");
    analyzerResolutionParam = apvts.getRawParameterValue ("analyzer_resolution");
    analyzerSpeedParam = apvts.getRawParameterValue ("analyzer_speed");
    analyzerTiltParam = apvts.getRawParameterValue ("analyzer_tilt");
    analyzerRangeParam = apvts.getRawParameterValue ("analyzer_range");
    analyzerReferenceDbParam = apvts.getRawParameterValue ("analyzer_reference_db");
    analyzerFreezeParam = apvts.getRawParameterValue ("analyzer_freeze");
    analyzerAutoRangeParam = apvts.getRawParameterValue ("analyzer_auto_range");
    noiseGT1.cacheParameters (apvts); eq4k.cacheParameters (apvts); comp76.cacheParameters (apvts); comp2a.cacheParameters (apvts); tube.cacheParameters (apvts); clipper.cacheParameters (apvts); esser.cacheParameters (apvts);
}

void D7SChannelStripFullAudioProcessor::syncAnalyzerParameters() noexcept
{
    analyzerState.preEnabled.store (readAtomic (analyzerPreEnabledParam, 1.0f) > 0.5f);
    analyzerState.postEnabled.store (readAtomic (analyzerPostEnabledParam, 1.0f) > 0.5f);
    analyzerState.resolutionIndex.store ((int) std::round (readAtomic (analyzerResolutionParam, 1.0f)));
    analyzerState.speedIndex.store ((int) std::round (readAtomic (analyzerSpeedParam, 3.0f)));
    analyzerState.rangeIndex.store ((int) std::round (readAtomic (analyzerRangeParam, 1.0f)));
    analyzerState.tiltDbPerOct.store (readAtomic (analyzerTiltParam, 4.5f));
    analyzerState.referenceDb.store (readAtomic (analyzerReferenceDbParam, -18.0f));
    analyzerState.freeze.store (readAtomic (analyzerFreezeParam, 0.0f) > 0.5f);
    analyzerState.autoRange.store (readAtomic (analyzerAutoRangeParam, 0.0f) > 0.5f);
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
    std::array<bool, numRackModules> used {};
    std::array<int, numRackModules> safe {};
    int write = 0;
    for (int i = 0; i < numRackModules; ++i)
    {
        const int id = newOrder[(size_t) i];
        if (id >= 0 && id < numRackModules && ! used[(size_t) id])
        {
            safe[(size_t) write++] = id;
            used[(size_t) id] = true;
        }
    }
    for (int id = 0; id < numRackModules; ++id)
        if (! used[(size_t) id])
            safe[(size_t) write++] = id;
    for (int i = 0; i < numRackModules; ++i)
        rackModuleOrder[(size_t) i].store (safe[(size_t) i]);
}

std::array<int, D7SChannelStripFullAudioProcessor::numRackModules> D7SChannelStripFullAudioProcessor::getModuleOrder() const noexcept
{
    std::array<int, numRackModules> result {};
    for (int i = 0; i < numRackModules; ++i)
        result[(size_t) i] = rackModuleOrder[(size_t) i].load();
    return result;
}

float D7SChannelStripFullAudioProcessor::getSpectrumBinDb (int index) const noexcept { return getPostSpectrumBinDb (index); }
float D7SChannelStripFullAudioProcessor::getPreSpectrumBinDb (int index) const noexcept { return (index < 0 || index >= numSpectrumBins) ? -120.0f : analyzerState.preDb[(size_t) index].load(); }
float D7SChannelStripFullAudioProcessor::getPostSpectrumBinDb (int index) const noexcept { return (index < 0 || index >= numSpectrumBins) ? -120.0f : analyzerState.postDb[(size_t) index].load(); }

void D7SChannelStripFullAudioProcessor::pushAnalyzerSample (juce::AbstractFifo& fifo, std::array<float, analyzerQueueSize>& queue, float sample) noexcept
{
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo.prepareToWrite (1, start1, size1, start2, size2);
    if (size1 > 0) queue[(size_t) start1] = sample;
    else if (size2 > 0) queue[(size_t) start2] = sample;
    fifo.finishedWrite (size1 + size2);
}

void D7SChannelStripFullAudioProcessor::pushSpectrumSample (float sample) noexcept { pushPostSpectrumSample (sample); }
void D7SChannelStripFullAudioProcessor::pushPreSpectrumSample (float sample) noexcept { if (analyzerState.preEnabled.load()) pushAnalyzerSample (preAnalyzerFifo, preAnalyzerQueue, sample); }
void D7SChannelStripFullAudioProcessor::pushPostSpectrumSample (float sample) noexcept { if (analyzerState.postEnabled.load()) pushAnalyzerSample (postAnalyzerFifo, postAnalyzerQueue, sample); }

void D7SChannelStripFullAudioProcessor::drainAnalyzerFifo (juce::AbstractFifo& fifo, std::array<float, analyzerQueueSize>& queue, std::array<float, spectrumFFTSize>& accumulation, int& accumulationIndex, bool pre)
{
    int available = fifo.getNumReady();
    while (available > 0 && ! threadShouldExit())
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        const int request = juce::jmin (available, 512);
        fifo.prepareToRead (request, start1, size1, start2, size2);
        auto consume = [&] (int start, int size)
        {
            for (int i = 0; i < size; ++i)
            {
                accumulation[(size_t) accumulationIndex++] = queue[(size_t) (start + i)];
                if (accumulationIndex >= spectrumFFTSize)
                {
                    if (pre) runPreSpectrumFFT(); else runPostSpectrumFFT();
                    accumulationIndex = 0;
                }
            }
        };
        consume (start1, size1); consume (start2, size2);
        fifo.finishedRead (size1 + size2);
        available -= (size1 + size2);
    }
}

void D7SChannelStripFullAudioProcessor::run()
{
    while (! threadShouldExit())
    {
        syncAnalyzerParameters();
        drainAnalyzerFifo (preAnalyzerFifo, preAnalyzerQueue, preSpectrumFifo, preSpectrumFifoIndex, true);
        drainAnalyzerFifo (postAnalyzerFifo, postAnalyzerQueue, postSpectrumFifo, postSpectrumFifoIndex, false);
        wait (16);
    }
}

void D7SChannelStripFullAudioProcessor::runSpectrumFFT() noexcept { runPostSpectrumFFT(); }

void D7SChannelStripFullAudioProcessor::runPreSpectrumFFT() noexcept
{
    if (analyzerState.freeze.load()) return;
    std::fill (preSpectrumFftData.begin(), preSpectrumFftData.end(), 0.0f);
    std::copy (preSpectrumFifo.begin(), preSpectrumFifo.end(), preSpectrumFftData.begin());
    spectrumWindow.multiplyWithWindowingTable (preSpectrumFftData.data(), spectrumFFTSize);
    spectrumFFT.performFrequencyOnlyForwardTransform (preSpectrumFftData.data(), true);
    const float nyquist = (float) currentSampleRate * 0.5f;
    const int fftBins = spectrumFFTSize / 2;
    const float release = analyzerReleaseCoeffForSpeed (analyzerState.speedIndex.load());
    for (int band = 0; band < numSpectrumBins; ++band)
    {
        const float f0 = logFrequencyForBin (band, numSpectrumBins);
        const float f1 = logFrequencyForBin (juce::jmin (band + 1, numSpectrumBins - 1), numSpectrumBins);
        const int bin0 = juce::jlimit (1, fftBins - 1, (int) std::floor ((f0 / nyquist) * (float) fftBins));
        const int bin1 = juce::jlimit (bin0 + 1, fftBins, (int) std::ceil ((f1 / nyquist) * (float) fftBins));
        float energy = 0.0f; int count = 0;
        for (int bin = bin0; bin < bin1; ++bin) { const float mag = preSpectrumFftData[(size_t) bin] / (float) spectrumFFTSize; energy += mag * mag; ++count; }
        const float rms = std::sqrt (energy / (float) juce::jmax (1, count));
        float targetDb = juce::Decibels::gainToDecibels (rms, -120.0f) + 42.0f;
        targetDb = juce::jlimit (-120.0f, 24.0f, targetDb);
        auto& smooth = preSpectrumSmoothedDb[(size_t) band];
        smooth += (targetDb > smooth ? 0.55f : release) * (targetDb - smooth);
        auto& peak = preSpectrumPeakDb[(size_t) band];
        if (smooth > peak) peak = smooth; else peak -= 0.40f;
        analyzerState.preDb[(size_t) band].store (smooth);
        analyzerState.prePeakDb[(size_t) band].store (peak);
    }
}

void D7SChannelStripFullAudioProcessor::runPostSpectrumFFT() noexcept
{
    if (analyzerState.freeze.load()) return;
    std::fill (postSpectrumFftData.begin(), postSpectrumFftData.end(), 0.0f);
    std::copy (postSpectrumFifo.begin(), postSpectrumFifo.end(), postSpectrumFftData.begin());
    spectrumWindow.multiplyWithWindowingTable (postSpectrumFftData.data(), spectrumFFTSize);
    spectrumFFT.performFrequencyOnlyForwardTransform (postSpectrumFftData.data(), true);
    const float nyquist = (float) currentSampleRate * 0.5f;
    const int fftBins = spectrumFFTSize / 2;
    const float release = analyzerReleaseCoeffForSpeed (analyzerState.speedIndex.load());
    for (int band = 0; band < numSpectrumBins; ++band)
    {
        const float f0 = logFrequencyForBin (band, numSpectrumBins);
        const float f1 = logFrequencyForBin (juce::jmin (band + 1, numSpectrumBins - 1), numSpectrumBins);
        const int bin0 = juce::jlimit (1, fftBins - 1, (int) std::floor ((f0 / nyquist) * (float) fftBins));
        const int bin1 = juce::jlimit (bin0 + 1, fftBins, (int) std::ceil ((f1 / nyquist) * (float) fftBins));
        float energy = 0.0f; int count = 0;
        for (int bin = bin0; bin < bin1; ++bin) { const float mag = postSpectrumFftData[(size_t) bin] / (float) spectrumFFTSize; energy += mag * mag; ++count; }
        const float rms = std::sqrt (energy / (float) juce::jmax (1, count));
        float targetDb = juce::Decibels::gainToDecibels (rms, -120.0f) + 42.0f;
        targetDb = juce::jlimit (-120.0f, 24.0f, targetDb);
        auto& smooth = postSpectrumSmoothedDb[(size_t) band];
        smooth += (targetDb > smooth ? 0.55f : release) * (targetDb - smooth);
        auto& peak = postSpectrumPeakDb[(size_t) band];
        if (smooth > peak) peak = smooth; else peak -= 0.40f;
        analyzerState.postDb[(size_t) band].store (smooth);
        analyzerState.postPeakDb[(size_t) band].store (peak);
        spectrumBinsDb[(size_t) band].store (smooth);
    }
}

void D7SChannelStripFullAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    noiseGT1.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels()); eq4k.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels()); comp76.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels()); comp2a.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels()); tube.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels()); clipper.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels()); esser.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels()); delayGlide.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    resetInternalStates();
    if (! isThreadRunning()) startThread (juce::Thread::Priority::low);
}

void D7SChannelStripFullAudioProcessor::releaseResources()
{
    stopThread (1000);
    noiseGT1.reset(); eq4k.reset(); comp76.reset(); comp2a.reset(); tube.reset(); clipper.reset(); esser.reset(); delayGlide.reset(); resetInternalStates();
}

void D7SChannelStripFullAudioProcessor::resetInternalStates()
{
    preAnalyzerFifo.reset(); postAnalyzerFifo.reset(); preAnalyzerQueue.fill (0.0f); postAnalyzerQueue.fill (0.0f);
    spectrumFifo.fill (0.0f); preSpectrumFifo.fill (0.0f); postSpectrumFifo.fill (0.0f);
    spectrumFftData.fill (0.0f); preSpectrumFftData.fill (0.0f); postSpectrumFftData.fill (0.0f);
    spectrumSmoothedDb.fill (-96.0f); spectrumPeakDb.fill (-96.0f); preSpectrumSmoothedDb.fill (-96.0f); postSpectrumSmoothedDb.fill (-96.0f); preSpectrumPeakDb.fill (-96.0f); postSpectrumPeakDb.fill (-96.0f);
    spectrumFifoIndex = 0; preSpectrumFifoIndex = 0; postSpectrumFifoIndex = 0;
    rackInputPeakDb.store (-120.0f); rackOutputPeakDb.store (-120.0f);
    analyzerState.reset(); for (auto& bin : spectrumBinsDb) bin.store (-96.0f);
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
        case moduleNoiseGT1: noiseGT1.process (buffer); break; case moduleEQ4K: eq4k.process (buffer); break; case module76: comp76.process (buffer); break; case module2A: comp2a.process (buffer); break; case moduleTube: tube.process (buffer); break; case moduleClipper: clipper.process (buffer); break; case moduleEsser: esser.process (buffer); break;
        case moduleDelay:
            delayGlide.setTempoBpm (bpm); delayGlide.setMix (getParam (apvts, "delay_mix", 25.0f)); delayGlide.setFeedback (getParam (apvts, "delay_feedback", 35.0f)); delayGlide.setDelayDivision (getChoiceParam (apvts, "delay_time", 3)); delayGlide.setDelayMode (getChoiceParam (apvts, "delay_mode", 1)); delayGlide.setDelayFractionIndex (getChoiceParam (apvts, "delay_fraction_index", 2)); delayGlide.setDelayTimeMs (getParam (apvts, "delay_time_ms", 250.0f)); delayGlide.setBypass (getBoolParam (apvts, "delay_bypass", true)); delayGlide.setGlideEnabled (getBoolParam (apvts, "delay_glide_on", false)); delayGlide.setGlideDirection (getChoiceParam (apvts, "delay_glide_direction", 0)); delayGlide.setGlideTime (getParam (apvts, "delay_glide_time", 35.0f)); delayGlide.process (buffer); break;
        default: break;
    }
}

template <typename FloatType>
void D7SChannelStripFullAudioProcessor::processAudioBlock (juce::AudioBuffer<FloatType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    syncAnalyzerParameters();
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel) buffer.clear (channel, 0, buffer.getNumSamples());
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
        auto* data = buffer.getWritePointer (ch); auto* dry = rackDry.getWritePointer (ch);
        for (int i = 0; i < n; ++i) { data[i] = (FloatType) ((double) data[i] * rackIn); dry[i] = data[i]; inputPeak = juce::jmax (inputPeak, std::abs ((double) data[i])); if (ch == 0) pushPreSpectrumSample ((float) data[i]); }
    }
    double bpm = 120.0;
    if (auto* playHead = getPlayHead()) { juce::AudioPlayHead::CurrentPositionInfo info; if (playHead->getCurrentPosition (info) && info.bpm > 0.0) bpm = info.bpm; }
    const auto order = getModuleOrder();
    for (int slot = 0; slot < numRackModules; ++slot) processModuleById (order[(size_t) slot], buffer, bpm);
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer (ch); auto* dry = rackDry.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            double x = (double) dry[i] * (1.0 - rackMix) + (double) data[i] * rackMix;
            x *= rackOut; outputPeak = juce::jmax (outputPeak, std::abs (x)); data[i] = (FloatType) juce::jlimit (-4.0, 4.0, x); if (ch == 0) pushPostSpectrumSample ((float) x);
        }
    }
    storeMeterWithDecay (rackInputPeakDb, (float) peakDbFromLinear (inputPeak));
    storeMeterWithDecay (rackOutputPeakDb, (float) peakDbFromLinear (outputPeak));
}

void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) { processAudioBlock (buffer); }
void D7SChannelStripFullAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&) { processAudioBlock (buffer); }
bool D7SChannelStripFullAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* D7SChannelStripFullAudioProcessor::createEditor() { return new D7SChannelStripFullAudioProcessorEditor (*this); }

void D7SChannelStripFullAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::String orderString; const auto order = getModuleOrder();
    for (int i = 0; i < numRackModules; ++i) { if (i > 0) orderString += ","; orderString += juce::String (order[(size_t) i]); }
    state.setProperty ("d7s_module_order", orderString, nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr) copyXmlToBinary (*xml, destData);
}

void D7SChannelStripFullAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        auto state = juce::ValueTree::fromXml (*xmlState);
        if (state.isValid())
        {
            const auto orderString = state.getProperty ("d7s_module_order").toString();
            if (orderString.isNotEmpty())
            {
                juce::StringArray tokens; tokens.addTokens (orderString, ",", "");
                if (tokens.size() == numRackModules)
                {
                    std::array<int, numRackModules> restored {};
                    for (int i = 0; i < numRackModules; ++i) restored[(size_t) i] = tokens[i].getIntValue();
                    setModuleOrder (restored);
                }
            }
            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new D7SChannelStripFullAudioProcessor(); }
