#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "../Core/IModule.h"

class EQ4KProcessor final : public IModule
{
public:
    void prepare (double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void process (juce::AudioBuffer<double>& buffer) override;
    void setBypass (bool shouldBypass) override;
    const juce::String getIdentifier() const override { return "eq4k"; }

    void cacheParameters (juce::AudioProcessorValueTreeState& apvts);

private:
    struct BiquadCoefficients
    {
        double b0 { 1.0 }, b1 { 0.0 }, b2 { 0.0 }, a1 { 0.0 }, a2 { 0.0 };
    };

    static double onePoleCoeff (double frequency, double sampleRate) noexcept;
    static BiquadCoefficients makePeakingEQ (double frequency, double q, double gainDb, double sampleRate) noexcept;
    static double processBiquad (double x, const BiquadCoefficients& c, double& z1, double& z2) noexcept;
    static double consoleSoftClip (double x, double drive01) noexcept;

    template <typename FloatType>
    void processInternal (juce::AudioBuffer<FloatType>& buffer);

    std::atomic<float>* hpfParam { nullptr };
    std::atomic<float>* lpfParam { nullptr };
    std::atomic<float>* lfFreqParam { nullptr };
    std::atomic<float>* lfGainParam { nullptr };
    std::atomic<float>* lfBellParam { nullptr };
    std::atomic<float>* lmfFreqParam { nullptr };
    std::atomic<float>* lmfGainParam { nullptr };
    std::atomic<float>* lmfQParam { nullptr };
    std::atomic<float>* hmfFreqParam { nullptr };
    std::atomic<float>* hmfGainParam { nullptr };
    std::atomic<float>* hmfQParam { nullptr };
    std::atomic<float>* hfFreqParam { nullptr };
    std::atomic<float>* hfGainParam { nullptr };
    std::atomic<float>* hfBellParam { nullptr };
    std::atomic<float>* driveParam { nullptr };
    std::atomic<float>* bypassParam { nullptr };

    double sr { 44100.0 };
    int channels { 2 };

    std::array<double, 8> hpfState {};
    std::array<double, 8> lpfState {};
    std::array<double, 8> lfState {};
    std::array<double, 8> lmfZ1 {};
    std::array<double, 8> lmfZ2 {};
    std::array<double, 8> hmfZ1 {};
    std::array<double, 8> hmfZ2 {};
    std::array<double, 8> hfState {};

    juce::SmoothedValue<float> hpfSmooth;
    juce::SmoothedValue<float> lpfSmooth;
    juce::SmoothedValue<float> lfFreqSmooth;
    juce::SmoothedValue<float> lfGainSmooth;
    juce::SmoothedValue<float> lmfFreqSmooth;
    juce::SmoothedValue<float> lmfGainSmooth;
    juce::SmoothedValue<float> lmfQSmooth;
    juce::SmoothedValue<float> hmfFreqSmooth;
    juce::SmoothedValue<float> hmfGainSmooth;
    juce::SmoothedValue<float> hmfQSmooth;
    juce::SmoothedValue<float> hfFreqSmooth;
    juce::SmoothedValue<float> hfGainSmooth;
    juce::SmoothedValue<float> driveSmooth;
    juce::SmoothedValue<float> bypassWet;
};
