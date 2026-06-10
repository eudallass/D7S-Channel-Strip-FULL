#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

#include "Core/SpectrumAnalyzer.h"
#include "Modules/NoiseGT1Processor.h"
#include "Modules/EQ4KProcessor.h"
#include "Modules/Comp76Processor.h"
#include "Modules/Comp2AProcessor.h"
#include "Modules/TubeProcessor.h"
#include "Modules/ClipperProcessor.h"
#include "Modules/EsserProcessor.h"
#include "Modules/DelayGlideProcessor.h"

class D7SChannelStripFullAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numRackModules = 8;

    enum ModuleIndex
    {
        moduleNoiseGT1 = 0,
        moduleEQ4K     = 1,
        module76       = 2,
        module2A       = 3,
        moduleTube     = 4,
        moduleClipper  = 5,
        moduleEsser    = 6,
        moduleDelay    = 7
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

    SpectrumAnalyzer preAnalyzer;
    SpectrumAnalyzer postAnalyzer;

    void setModuleOrder (const std::array<int, numRackModules>& newOrder) noexcept;
    std::array<int, numRackModules> getModuleOrder() const noexcept;

    float getRackInputPeakDb() const noexcept  { return rackInputPeakDb.load(); }
    float getRackOutputPeakDb() const noexcept { return rackOutputPeakDb.load(); }
    float getNoiseGT1GainReductionDb() const noexcept { return noiseGT1.getGainReductionDb(); }
    float getComp76GainReductionDb() const noexcept { return comp76.getGainReductionDb(); }
    float getComp2AGainReductionDb() const noexcept { return comp2a.getGainReductionDb(); }
    float getEsserGainReductionDb() const noexcept { return esser.getGainReductionDb(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    template <typename FloatType>
    void processAudioBlock (juce::AudioBuffer<FloatType>& buffer);

    template <typename FloatType>
    void pushAnalyzerBuffer (SpectrumAnalyzer& analyzer, juce::AudioBuffer<FloatType>& buffer) noexcept;

    template <typename FloatType>
    void processModuleById (int moduleId, juce::AudioBuffer<FloatType>& buffer, double bpm);

    void cacheParameterPointers();
    void resetInternalStates();

    juce::AudioProcessorValueTreeState apvts;

    NoiseGT1Processor noiseGT1;
    EQ4KProcessor eq4k;
    Comp76Processor comp76;
    Comp2AProcessor comp2a;
    TubeProcessor tube;
    ClipperProcessor clipper;
    EsserProcessor esser;
    DelayGlideProcessor delayGlide;

    std::atomic<float>* rackInputParam { nullptr };
    std::atomic<float>* rackOutputParam { nullptr };
    std::atomic<float>* rackMixParam { nullptr };
    std::atomic<float>* masterBypassParam { nullptr };

    double currentSampleRate { 44100.0 };
    std::array<std::atomic<int>, numRackModules> rackModuleOrder {};

    std::atomic<float> rackInputPeakDb  { -120.0f };
    std::atomic<float> rackOutputPeakDb { -120.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessor)
};
