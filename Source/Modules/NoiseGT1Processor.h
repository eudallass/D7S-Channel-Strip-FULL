#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class NoiseGT1Processor
{
public:
    NoiseGT1Processor() = default;
    ~NoiseGT1Processor() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process (juce::AudioBuffer<float>& buffer);
    void process (juce::AudioBuffer<double>& buffer);

    void cacheParameters (juce::AudioProcessorValueTreeState& apvts);
    void setSuppressionAmount (float zeroToOne);
    void setBypass (bool shouldBypass);

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
