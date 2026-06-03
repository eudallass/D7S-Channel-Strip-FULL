#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "Modules/NoiseGT1Processor.h"
#include "Modules/DelayGlideProcessor.h"

class D7SChannelStripFullAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int spectrumOrder = 11;
    static constexpr int spectrumFFTSize = 1 << spectrumOrder;
    static constexpr int numSpectrumBins = 96;
    static constexpr int numRackModules = 6;

    enum ModuleIndex
    {
        moduleNoiseGT1 = 0,
        moduleEQ4K     = 1,
        module76       = 2,
        module2A       = 3,
        moduleTube     = 4,
        moduleEsser    = 5
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

    void setModuleOrder (const std::array<int, numRackModules>& newOrder) noexcept;
    std::array<int, numRackModules> getModuleOrder() const noexcept;

    float getRackInputPeakDb() const noexcept  { return rackInputPeakDb.load(); }
    float getRackOutputPeakDb() const noexcept { return rackOutputPeakDb.load(); }
    float getNoiseGT1GainReductionDb() const noexcept { return noiseGT1.getGainReductionDb(); }
    float getComp76GainReductionDb() const noexcept { return comp76GainReductionDb.load(); }
    float getComp2AGainReductionDb() const noexcept { return comp2aGainReductionDb.load(); }
    float getEsserGainReductionDb() const noexcept { return esserGainReductionDb.load(); }
    float getSpectrumBinDb (int index) const noexcept;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    template <typename FloatType>
    void processAudioBlock (juce::AudioBuffer<FloatType>& buffer);

    void pushSpectrumSample (float sample) noexcept;
    void runSpectrumFFT() noexcept;
    void resetInternalStates();

    juce::AudioProcessorValueTreeState apvts;

    NoiseGT1Processor noiseGT1;
    DelayGlideProcessor delayGlide;

    double currentSampleRate { 44100.0 };

    std::array<std::atomic<int>, numRackModules> rackModuleOrder {};

    std::array<double, 8> eqHpfState {};
    std::array<double, 8> eqLpfState {};
    std::array<double, 8> eqLfState  {};
    std::array<double, 8> eqLmfLow   {};
    std::array<double, 8> eqLmfHigh  {};
    std::array<double, 8> eqHmfLow   {};
    std::array<double, 8> eqHmfHigh  {};
    std::array<double, 8> eqHfState  {};

    std::array<double, 8> comp76Env {};
    std::array<double, 8> comp2aEnv {};

    std::array<double, 8> tubeBeautyState {};
    std::array<double, 8> tubeBeastState  {};

    std::array<double, 8> esserLp  {};
    std::array<double, 8> esserEnv {};

    juce::dsp::FFT spectrumFFT { spectrumOrder };
    juce::dsp::WindowingFunction<float> spectrumWindow { spectrumFFTSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, spectrumFFTSize> spectrumFifo {};
    std::array<float, spectrumFFTSize * 2> spectrumFftData {};
    std::array<float, numSpectrumBins> spectrumSmoothedDb {};
    std::array<float, numSpectrumBins> spectrumPeakDb {};
    std::array<std::atomic<float>, numSpectrumBins> spectrumBinsDb {};
    int spectrumFifoIndex { 0 };

    std::atomic<float> rackInputPeakDb  { -120.0f };
    std::atomic<float> rackOutputPeakDb { -120.0f };
    std::atomic<float> comp76GainReductionDb { 0.0f };
    std::atomic<float> comp2aGainReductionDb { 0.0f };
    std::atomic<float> esserGainReductionDb  { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessor)
};
