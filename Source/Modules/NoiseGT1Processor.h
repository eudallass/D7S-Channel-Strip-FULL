#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

/**
 * D7S NoiseGT1 -- Gate inteligente inspirado no Waves NS1 Noise Suppressor
 *
 * Filosofia: um knob so (Suppression), operacao adaptativa, sem pumping.
 * O detector usa RMS com duas constantes de tempo (analise lenta de noise floor
 * + envelope rapido do sinal) pra decidir quanto gain reduction aplicar.
 *
 * Signal flow:
 *   Input -> RMS Detector -> Gain Computer -> Gain Smoother -> VCA -> Output
 */
class NoiseGT1Processor
{
public:
    NoiseGT1Processor() = default;
    ~NoiseGT1Processor() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    template <typename FloatType>
    void process (juce::AudioBuffer<FloatType>& buffer);

    void setSuppressionAmount (float zeroToOne);
    void setBypass (bool shouldBypass);

    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

private:
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

    std::atomic<float> gainReductionDb { 0.0f };

    static float computeAttackCoeff  (float timeMs, double sr) noexcept;
    static float computeReleaseCoeff (float timeMs, double sr) noexcept;

    float computeThreshold() const noexcept;
    float computeGainTarget (float signalRms, float noiseRms) const noexcept;
};
