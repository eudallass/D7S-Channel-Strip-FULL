#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "../Core/IModule.h"

class EsserProcessor final : public IModule
{
public:
    void prepare (double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void process (juce::AudioBuffer<double>& buffer) override;
    void setBypass (bool shouldBypass) override;
    const juce::String getIdentifier() const override { return "esser"; }

    void cacheParameters (juce::AudioProcessorValueTreeState& apvts);
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

private:
    template <typename FloatType>
    void processInternal (juce::AudioBuffer<FloatType>& buffer);
    static double onePoleCoeff (double frequency, double sampleRate) noexcept;
    static double smoothCoeffMs (double timeMs, double sampleRate) noexcept;
    static double peakDbFromLinear (double value) noexcept;

    std::atomic<float>* thresholdParam { nullptr };
    std::atomic<float>* freqParam { nullptr };
    std::atomic<float>* rangeParam { nullptr };
    std::atomic<float>* modeParam { nullptr };
    std::atomic<float>* bypassParam { nullptr };

    double sr { 44100.0 };
    int channels { 2 };
    std::array<double, 8> lp {};
    std::array<double, 8> env {};

    juce::SmoothedValue<float> thresholdSmooth;
    juce::SmoothedValue<float> freqSmooth;
    juce::SmoothedValue<float> rangeSmooth;
    juce::SmoothedValue<float> bypassWet;

    std::atomic<float> gainReductionDb { 0.0f };
};
