#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

#include "Core/AnalyzerState.h"
#include "Modules/NoiseGT1Processor.h"
#include "Modules/EQ4KProcessor.h"
#include "Modules/Comp76Processor.h"
#include "Modules/Comp2AProcessor.h"
#include "Modules/TubeProcessor.h"
#include "Modules/EsserProcessor.h"
#include "Modules/DelayGlideProcessor.h"

class D7SChannelStripFullAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int spectrumOrder = 11;
    static constexpr int spectrumFFTSize = 1 << spectrumOrder;
    static constexpr int numSpectrumBins = 96;
    static constexpr int numRackModules = 7;

    enum ModuleIndex
    {
        moduleNoiseGT1 = 0,
        moduleEQ4K     = 1,
        module76       = 2,
        module2A       = 3,
        moduleTube     = 4,
        moduleEsser    = 5,
        moduleDelay    = 6
    };

    D7SChannelStripFullAudioProcessor();
    ~D7SChannelStripFullAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }
    juce::AudioProcessorParameter* getBypassParameter() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    const AnalyzerState& getAnalyzerState() const noexcept { return analyzerState; }

    void setModuleOrder (const std::array<int, numRackModules>& newOrder) noexcept;
    std::array<int, numRackModules> getModuleOrder() const noexcept;

    float getRackInputPeakDb() const noexcept  { return rackInputPeakDb.load(); }
    float getRackOutputPeakDb() const noexcept { return rackOutputPeakDb.load(); }
    float getNoiseGT1GainReductionDb() const noexcept { return noiseGT1.getGainReductionDb(); }
    float getComp76GainReductionDb() const noexcept { return comp76.getGainReductionDb(); }
    float getComp2AGainReductionDb() const noexcept { return comp2a.getGainReductionDb(); }
    float getEsserGainReductionDb() const noexcept { return esser.getGainReductionDb(); }
    float getSpectrumBinDb (int index) const noexcept;
    float getPreSpectrumBinDb (int index) const noexcept;
    float getPostSpectrumBinDb (int index) const noexcept;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    template <typename FloatType>
    void processAudioBlock (juce::AudioBuffer<FloatType>& buffer);

    template <typename FloatType>
    void processModuleById (int moduleId, juce::AudioBuffer<FloatType>& buffer, double bpm);

    void cacheParameterPointers();
    void syncAnalyzerParameters() noexcept;
    void pushSpectrumSample (float sample) noexcept;
    void pushPreSpectrumSample (float sample) noexcept;
    void pushPostSpectrumSample (float sample) noexcept;
    void runSpectrumFFT() noexcept;
    void runPreSpectrumFFT() noexcept;
    void runPostSpectrumFFT() noexcept;
    void resetInternalStates();

    juce::AudioProcessorValueTreeState apvts;

    NoiseGT1Processor noiseGT1;
    EQ4KProcessor eq4k;
    Comp76Processor comp76;
    Comp2AProcessor comp2a;
    TubeProcessor tube;
    EsserProcessor esser;
    DelayGlideProcessor delayGlide;

    std::atomic<float>* rackInputParam { nullptr };
    std::atomic<float>* rackOutputParam { nullptr };
    std::atomic<float>* rackMixParam { nullptr };
    std::atomic<float>* masterBypassParam { nullptr };
    std::atomic<float>* analyzerPreEnabledParam { nullptr };
    std::atomic<float>* analyzerPostEnabledParam { nullptr };
    std::atomic<float>* analyzerResolutionParam { nullptr };
    std::atomic<float>* analyzerSpeedParam { nullptr };
    std::atomic<float>* analyzerTiltParam { nullptr };
    std::atomic<float>* analyzerRangeParam { nullptr };
    std::atomic<float>* analyzerReferenceDbParam { nullptr };
    std::atomic<float>* analyzerFreezeParam { nullptr };
    std::atomic<float>* analyzerAutoRangeParam { nullptr };

    double currentSampleRate { 44100.0 };
    std::array<std::atomic<int>, numRackModules> rackModuleOrder {};

    AnalyzerState analyzerState;

    juce::dsp::FFT spectrumFFT { spectrumOrder };
    juce::dsp::WindowingFunction<float> spectrumWindow { spectrumFFTSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, spectrumFFTSize> spectrumFifo {};
    std::array<float, spectrumFFTSize> preSpectrumFifo {};
    std::array<float, spectrumFFTSize> postSpectrumFifo {};
    std::array<float, spectrumFFTSize * 2> spectrumFftData {};
    std::array<float, spectrumFFTSize * 2> preSpectrumFftData {};
    std::array<float, spectrumFFTSize * 2> postSpectrumFftData {};
    std::array<float, numSpectrumBins> spectrumSmoothedDb {};
    std::array<float, numSpectrumBins> spectrumPeakDb {};
    std::array<float, numSpectrumBins> preSpectrumSmoothedDb {};
    std::array<float, numSpectrumBins> postSpectrumSmoothedDb {};
    std::array<float, numSpectrumBins> preSpectrumPeakDb {};
    std::array<float, numSpectrumBins> postSpectrumPeakDb {};
    std::array<std::atomic<float>, numSpectrumBins> spectrumBinsDb {};
    int spectrumFifoIndex { 0 };
    int preSpectrumFifoIndex { 0 };
    int postSpectrumFifoIndex { 0 };

    std::atomic<float> rackInputPeakDb  { -120.0f };
    std::atomic<float> rackOutputPeakDb { -120.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessor)
};
