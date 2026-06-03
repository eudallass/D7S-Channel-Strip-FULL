#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <random>

class DelayGlideProcessor
{
public:
    enum DelayTimeDivision
    {
        div_1_32 = 0,
        div_1_16,
        div_1_8,
        div_1_4,
        div_1_2,
        div_1_1,
        div_1_8T,
        div_1_4T,
        div_1_8D,
        div_1_4D
    };

    enum GlideDirection
    {
        glideUp = 0,
        glideDown,
        glideRandom
    };

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void setMix (float percent) noexcept;
    void setFeedback (float percent) noexcept;
    void setDelayDivision (int division) noexcept;
    void setBypass (bool shouldBypass) noexcept;
    void setGlideEnabled (bool enabled) noexcept;
    void setGlideDirection (int direction) noexcept;
    void setGlideTime (float percent) noexcept;
    void setTempoBpm (double bpm) noexcept;

    template <typename FloatType>
    void process (juce::AudioBuffer<FloatType>& buffer);

private:
    static constexpr int maxChannels = 2;
    static constexpr int numLines = 8;
    static constexpr double maxDelaySeconds = 2.2;

    struct DelayLine
    {
        std::vector<float> buffer;
        int writeIndex { 0 };
    };

    float readDelayLineLinear (DelayLine& line, float delaySamples) const noexcept;
    void writeDelayLine (DelayLine& line, float value) noexcept;
    float getDivisionBeats() const noexcept;
    float getGlidePitchRatio() noexcept;
    void advanceGlidePhase() noexcept;

    double sr { 44100.0 };
    int channels { 2 };
    int maxDelaySamples { 1 };

    float mix { 0.25f };
    float feedback { 0.35f };
    int delayDivision { div_1_4 };
    bool bypassed { true };
    bool glideEnabled { false };
    int glideDirection { glideUp };
    float glideTime { 0.35f };
    double tempoBpm { 120.0 };

    std::array<std::array<DelayLine, numLines>, maxChannels> lines;
    std::array<float, numLines> lineOutputs {};
    std::array<float, numLines> feedbackVector {};

    // Gemini-like low-density FDN: prime-ish base times spread across up to 2 seconds.
    std::array<float, numLines> baseMs { 67.0f, 89.0f, 103.0f, 127.0f, 149.0f, 173.0f, 191.0f, 211.0f };
    std::array<float, numLines> lfoPhase { 0.0f, 0.17f, 0.29f, 0.41f, 0.53f, 0.67f, 0.79f, 0.91f };

    float glidePhase { 0.0f };
    int currentRandomDirection { 1 };
    std::mt19937 rng { 777 };
};
