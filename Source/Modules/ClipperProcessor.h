#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "../Core/IModule.h"

class ClipperProcessor final : public IModule
{
public:
    void prepare (double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void process (juce::AudioBuffer<double>& buffer) override;
    void setBypass (bool shouldBypass) override;
    int getLatencySamples() const noexcept override { return 0; }
    const juce::String getIdentifier() const override { return "clipper"; }

    void cacheParameters (juce::AudioProcessorValueTreeState& apvts);

private:
    template <typename FloatType>
    void processInternal (juce::AudioBuffer<FloatType>& buffer) noexcept;

    static float fsClip (float x, float thresholdLinear) noexcept;
    float processDcBlocker (int channel, float x) noexcept;

    std::atomic<float>* thresholdParam { nullptr };
    std::atomic<float>* preParam { nullptr };
    std::atomic<float>* postParam { nullptr };
    std::atomic<float>* bypassParam { nullptr };

    double sr { 44100.0 };
    int channels { 2 };
    float dcCoef { 0.0f };
    std::array<float, 8> dcX1 {};
    std::array<float, 8> dcY1 {};

    juce::SmoothedValue<float> thresholdLinearSmooth;
    juce::SmoothedValue<float> preGainLinearSmooth;
    juce::SmoothedValue<float> postGainLinearSmooth;
    juce::SmoothedValue<float> bypassWet;
};
