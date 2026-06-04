#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "../Core/IModule.h"

class Comp2AProcessor final : public IModule
{
public:
    void prepare (double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void process (juce::AudioBuffer<double>& buffer) override;
    void setBypass (bool shouldBypass) override;
    const juce::String getIdentifier() const override { return "comp2a"; }

    void cacheParameters (juce::AudioProcessorValueTreeState& apvts);
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

private:
    template <typename FloatType>
    void processInternal (juce::AudioBuffer<FloatType>& buffer);
    static double smoothCoeffMs (double timeMs, double sampleRate) noexcept;
    static double peakDbFromLinear (double value) noexcept;
    double t4OptoResponseDb (int channel, double controlVoltageDb) noexcept;
    double processSidechainHPF (int channel, double x) noexcept;

    std::atomic<float>* peakParam { nullptr };
    std::atomic<float>* gainParam { nullptr };
    std::atomic<float>* modeParam { nullptr };
    std::atomic<float>* emphasisParam { nullptr };
    std::atomic<float>* mixParam { nullptr };
    std::atomic<float>* scHpfParam { nullptr };
    std::atomic<float>* bypassParam { nullptr };

    double sr { 44100.0 };
    int channels { 2 };
    std::array<double, 8> env {};
    std::array<double, 8> cellMemory {};
    std::array<double, 8> scHpfX1 {};
    std::array<double, 8> scHpfY1 {};
    int scHpfIndex { 0 };

    juce::SmoothedValue<float> peakSmooth;
    juce::SmoothedValue<float> gainSmooth;
    juce::SmoothedValue<float> emphasisSmooth;
    juce::SmoothedValue<float> mixSmooth;
    juce::SmoothedValue<float> bypassWet;

    std::atomic<float> gainReductionDb { 0.0f };
};
