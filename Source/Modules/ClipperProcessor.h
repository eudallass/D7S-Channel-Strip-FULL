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
    const juce::String getIdentifier() const override { return "clipper"; }

    void cacheParameters (juce::AudioProcessorValueTreeState& apvts);

private:
    template <typename FloatType>
    void processInternal (juce::AudioBuffer<FloatType>& buffer);

    static double hybridClip (double x, double shape, double bias) noexcept;
    double processDcBlocker (int channel, double x) noexcept;

    std::atomic<float>* driveParam { nullptr };
    std::atomic<float>* ceilingParam { nullptr };
    std::atomic<float>* shapeParam { nullptr };
    std::atomic<float>* mixParam { nullptr };
    std::atomic<float>* bypassParam { nullptr };

    double sr { 44100.0 };
    int channels { 2 };
    double dcCoef { 0.0 };
    double biasPhase { 0.0 };
    std::array<double, 8> dcX1 {};
    std::array<double, 8> dcY1 {};

    juce::SmoothedValue<float> driveSmooth;
    juce::SmoothedValue<float> ceilingSmooth;
    juce::SmoothedValue<float> shapeSmooth;
    juce::SmoothedValue<float> mixSmooth;
    juce::SmoothedValue<float> bypassWet;
};
