#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "Modules/NoiseGT1Processor.h"

class D7SChannelStripFullAudioProcessor : public juce::AudioProcessor
{
public:
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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    template <typename FloatType>
    void processAudioBlock (juce::AudioBuffer<FloatType>& buffer);

    void resetInternalStates();

    juce::AudioProcessorValueTreeState apvts;

    NoiseGT1Processor noiseGT1;

    double currentSampleRate { 44100.0 };

    std::array<double, 8> eqLp100  {};
    std::array<double, 8> eqLp500  {};
    std::array<double, 8> eqLp1500 {};
    std::array<double, 8> eqLp3000 {};
    std::array<double, 8> eqLp6000 {};

    std::array<double, 8> comp76Env {};
    std::array<double, 8> comp2aEnv {};

    std::array<double, 8> esserLp  {};
    std::array<double, 8> esserEnv {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D7SChannelStripFullAudioProcessor)
};
