#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "../Core/IModule.h"

class TubeProcessor final : public IModule
{
public:
    void prepare (double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void process (juce::AudioBuffer<double>& buffer) override;
    void setBypass (bool shouldBypass) override;
    int getLatencySamples() const noexcept override { return oversampleLatency; }
    const juce::String getIdentifier() const override { return "tube"; }

    void cacheParameters (juce::AudioProcessorValueTreeState& apvts);

private:
    template <typename FloatType>
    void processInternal (juce::AudioBuffer<FloatType>& buffer);
    static double onePoleCoeff (double frequency, double sampleRate) noexcept;
    static double tubeWaveshape (double x, double bias, double drive) noexcept;
    double computeBiasDrift() noexcept;
    double processDcBlocker (int channel, double x) noexcept;

    std::atomic<float>* beautyParam { nullptr };
    std::atomic<float>* beastParam { nullptr };
    std::atomic<float>* sensitivityParam { nullptr };
    std::atomic<float>* mixParam { nullptr };
    std::atomic<float>* bypassParam { nullptr };

    double sr { 44100.0 };
    int channels { 2 };
    int oversampleLatency { 0 };
    double dcBlockerCoef { 0.0 };
    double biasDriftPhase { 0.0 };
    juce::Random biasJitterRng;
    std::array<double, 8> beautyState {};
    std::array<double, 8> beastState {};
    std::array<double, 8> dcX1 {};
    std::array<double, 8> dcY1 {};

    juce::SmoothedValue<float> beautySmooth;
    juce::SmoothedValue<float> beastSmooth;
    juce::SmoothedValue<float> sensitivitySmooth;
    juce::SmoothedValue<float> mixSmooth;
    juce::SmoothedValue<float> bypassWet;
};
