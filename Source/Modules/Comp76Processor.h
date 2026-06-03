#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "../Core/IModule.h"

class Comp76Processor final : public IModule
{
public:
    void prepare (double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void process (juce::AudioBuffer<double>& buffer) override;
    void setBypass (bool shouldBypass) override;
    int getLatencySamples() const noexcept override { return lookaheadSamples + oversampleLatency; }
    const juce::String getIdentifier() const override { return "comp76"; }

    void cacheParameters (juce::AudioProcessorValueTreeState& apvts);
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

private:
    template <typename FloatType>
    void processInternal (juce::AudioBuffer<FloatType>& buffer);

    static double smoothCoeffMs (double timeMs, double sampleRate) noexcept;
    static double peakDbFromLinear (double value) noexcept;

    std::atomic<float>* inputParam { nullptr };
    std::atomic<float>* outputParam { nullptr };
    std::atomic<float>* attackParam { nullptr };
    std::atomic<float>* releaseParam { nullptr };
    std::atomic<float>* ratioParam { nullptr };
    std::atomic<float>* bypassParam { nullptr };

    double sr { 44100.0 };
    int channels { 2 };
    int lookaheadSamples { 0 };
    int oversampleLatency { 0 };
    std::array<double, 8> env {};

    juce::SmoothedValue<float> inputSmooth;
    juce::SmoothedValue<float> outputSmooth;
    juce::SmoothedValue<float> attackSmooth;
    juce::SmoothedValue<float> releaseSmooth;
    juce::SmoothedValue<float> bypassWet;

    std::atomic<float> gainReductionDb { 0.0f };
};
