#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "../Core/IModule.h"

class NoiseGT1Processor final : public IModule
{
public:
    NoiseGT1Processor() = default;
    ~NoiseGT1Processor() override = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels) override;
    void reset() override;

    void process (juce::AudioBuffer<float>& buffer) override;
    void process (juce::AudioBuffer<double>& buffer) override;

    void cacheParameters (juce::AudioProcessorValueTreeState& apvts);
    void setSuppressionAmount (float zeroToOne);
    void setBypass (bool shouldBypass) override;
    int getLatencySamples() const noexcept override { return lookaheadSamples; }
    const juce::String getIdentifier() const override { return "noisegt1"; }

    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

private:
    template <typename FloatType>
    void processInternal (juce::AudioBuffer<FloatType>& buffer);

    std::atomic<float>* suppressionParam { nullptr };
    std::atomic<float>* bypassParam { nullptr };

    float suppressionAmount { 0.5f };
    bool  bypassed          { false };

    double sampleRate    { 44100.0 };
    int    numChannels   { 2 };
    int lookaheadSamples { 0 };

    std::array<float, 8> signalEnvelope {};
    float signalAttackCoeff  { 0.0f };
    float signalReleaseCoeff { 0.0f };

    std::array<float, 8> noiseFloorEstimate {};
    float noiseAttackCoeff  { 0.0f };
    float noiseReleaseCoeff { 0.0f };

    float currentGain      { 1.0f };
    float gainAttackCoeff  { 0.0f };
    float gainReleaseCoeff { 0.0f };

    juce::SmoothedValue<float> bypassWet;
    juce::SmoothedValue<float> suppressionSmooth;

    std::atomic<float> gainReductionDb { 0.0f };

    static float computeAttackCoeff  (float timeMs, double sr) noexcept;
    static float computeReleaseCoeff (float timeMs, double sr) noexcept;

    float computeThreshold() const noexcept;
    float computeGainTarget (float signalRms, float noiseRms) const noexcept;
};
