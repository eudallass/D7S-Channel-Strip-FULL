#include "DelayGlideProcessor.h"
#include <cmath>

namespace
{
    constexpr float antiDenormalConst = 1.0e-25f;
}

void DelayGlideProcessor::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, maxChannels, numChannels);
    preparedBlockSize = juce::jmax (1, samplesPerBlock);
    maxDelaySamples = (int) std::ceil (maxDelaySeconds * sr) + 32;
    glideCoef = 1.0f - std::exp (-1.0f / (0.100f * (float) juce::jmax (sr, 1.0)));

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) preparedBlockSize, 1 };
    for (auto& channelLines : lines)
    {
        for (auto& line : channelLines)
        {
            line.setMaximumDelayInSamples (maxDelaySamples);
            line.prepare (spec);
        }
    }

    reset();
}

void DelayGlideProcessor::reset()
{
    for (auto& channelLines : lines)
        for (auto& line : channelLines)
            line.reset();

    for (auto& chState : currentDelaySamples)
        chState.fill (0.0f);
    for (auto& chState : targetDelaySamplesState)
        chState.fill (0.0f);

    lineOutputs.fill (0.0f);
    feedbackVector.fill (0.0f);
    wowPhase = 0.0f;
    flutterPhase = 0.0f;
    glidePhase = 0.0f;
    currentRandomDirection = 1;
}

void DelayGlideProcessor::setMix (float percent) noexcept { mix = juce::jlimit (0.0f, 1.0f, percent / 100.0f); }
void DelayGlideProcessor::setFeedback (float percent) noexcept { feedback = juce::jlimit (0.0f, 0.95f, percent / 100.0f * 0.95f); }
void DelayGlideProcessor::setDelayDivision (int division) noexcept { delayDivision = juce::jlimit (0, 9, division); }
void DelayGlideProcessor::setDelayMode (int mode) noexcept { delayMode = juce::jlimit (0, 3, mode); }
void DelayGlideProcessor::setDelayFractionIndex (int index) noexcept { delayFractionIndex = juce::jlimit (0, 6, index); }
void DelayGlideProcessor::setDelayTimeMs (float ms) noexcept { delayTimeMs = juce::jlimit (0.1f, 3000.0f, ms); }
void DelayGlideProcessor::setBypass (bool shouldBypass) noexcept { bypassed = shouldBypass; }
void DelayGlideProcessor::setGlideEnabled (bool enabled) noexcept { glideEnabled = enabled; }
void DelayGlideProcessor::setGlideDirection (int direction) noexcept { glideDirection = juce::jlimit (0, 2, direction); }
void DelayGlideProcessor::setGlideTime (float percent) noexcept { glideTime = juce::jlimit (0.0f, 1.0f, percent / 100.0f); }
void DelayGlideProcessor::setTempoBpm (double bpm) noexcept { tempoBpm = juce::jlimit (30.0, 300.0, bpm > 0.0 ? bpm : 120.0); }

float DelayGlideProcessor::getDivisionBeats() const noexcept
{
    switch (delayDivision)
    {
        case div_1_32: return 0.125f; case div_1_16: return 0.25f; case div_1_8: return 0.5f; case div_1_4: return 1.0f; case div_1_2: return 2.0f; case div_1_1: return 4.0f; case div_1_8T: return 1.0f / 3.0f; case div_1_4T: return 2.0f / 3.0f; case div_1_8D: return 0.75f; case div_1_4D: return 1.5f; default: return 1.0f;
    }
}

float DelayGlideProcessor::getModeBeats() const noexcept
{
    static constexpr float noteBeats[7] { 1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 4.0f, 1.0f / 2.0f, 1.0f, 2.0f, 4.0f };
    static constexpr float dottedBeats[5] { 1.0f / 8.0f * 1.5f, 1.0f / 4.0f * 1.5f, 1.0f / 2.0f * 1.5f, 1.0f * 1.5f, 2.0f * 1.5f };
    static constexpr float tripletBeats[5] { 1.0f / 8.0f * 2.0f / 3.0f, 1.0f / 4.0f * 2.0f / 3.0f, 1.0f / 2.0f * 2.0f / 3.0f, 1.0f * 2.0f / 3.0f, 2.0f * 2.0f / 3.0f };
    if (delayMode == 2) return dottedBeats[juce::jlimit (0, 4, delayFractionIndex)];
    if (delayMode == 3) return tripletBeats[juce::jlimit (0, 4, delayFractionIndex)];
    return noteBeats[juce::jlimit (0, 6, delayFractionIndex)];
}

float DelayGlideProcessor::getUserDelaySeconds() const noexcept
{
    if (delayMode == 0)
        return juce::jlimit (0.0001f, 3.0f, delayTimeMs * 0.001f);
    return juce::jlimit (0.02f, 2.0f, (60.0f / (float) tempoBpm) * getModeBeats());
}

float DelayGlideProcessor::readTapeLine (int ch, int lineIndex, float targetDelaySamples) noexcept
{
    auto& line = lines[(size_t) ch][(size_t) lineIndex];
    auto& current = currentDelaySamples[(size_t) ch][(size_t) lineIndex];
    auto& target = targetDelaySamplesState[(size_t) ch][(size_t) lineIndex];
    target = juce::jlimit (1.0f, (float) maxDelaySamples - 4.0f, targetDelaySamples);
    if (current <= 0.0f) current = target;
    current += glideCoef * (target - current);
    line.setDelay (current);
    return line.popSample (0);
}

void DelayGlideProcessor::writeTapeLine (TapeDelayLine& line, float value) noexcept { line.pushSample (0, value); }

float DelayGlideProcessor::getGlidePitchRatio() noexcept
{
    if (! glideEnabled) return 1.0f;
    const float activePortion = juce::jmax (0.02f, glideTime);
    const float ramp = juce::jlimit (0.0f, 1.0f, glidePhase / activePortion);
    int dir = 1; if (glideDirection == glideDown) dir = -1; else if (glideDirection == glideRandom) dir = currentRandomDirection;
    return std::pow (2.0f, (12.0f * ramp * (float) dir) / 12.0f);
}

float DelayGlideProcessor::getWowFlutterMultiplier() noexcept
{
    constexpr float wowRate = 0.6f, flutterRate = 5.5f, wowDepth = 0.002f, flutterDepth = 0.0008f;
    wowPhase += wowRate / (float) juce::jmax (sr, 1.0); if (wowPhase >= 1.0f) wowPhase -= 1.0f;
    flutterPhase += flutterRate / (float) juce::jmax (sr, 1.0); if (flutterPhase >= 1.0f) flutterPhase -= 1.0f;
    const float wow = std::sin (wowPhase * juce::MathConstants<float>::twoPi) * wowDepth;
    const float flutterSine = std::sin (flutterPhase * juce::MathConstants<float>::twoPi) * flutterDepth * 0.45f;
    const float flutterNoise = flutterDistribution (flutterRng) * flutterDepth * 0.55f;
    return 1.0f + wow + flutterSine + flutterNoise;
}

void DelayGlideProcessor::advanceGlidePhase() noexcept
{
    const float cycleSeconds = juce::jmax (0.02f, (60.0f / (float) tempoBpm) * getModeBeats());
    const float oldPhase = glidePhase;
    glidePhase += 1.0f / (cycleSeconds * (float) sr);
    if (glidePhase >= 1.0f)
    {
        glidePhase -= std::floor (glidePhase);
        if (glideDirection == glideRandom || oldPhase > glidePhase)
            currentRandomDirection = (std::uniform_int_distribution<int> (0, 1) (rng) == 0) ? -1 : 1;
    }
}

template <typename FloatType>
void DelayGlideProcessor::process (juce::AudioBuffer<FloatType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    if (bypassed || mix <= 0.0001f) return;

    const int numCh = juce::jmin (channels, buffer.getNumChannels());
    const int numSamples = buffer.getNumSamples();
    const float userDelaySeconds = getUserDelaySeconds();
    float antiDenormalSign = 1.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        antiDenormalSign = -antiDenormalSign;
        const float antiDenormalNoise = antiDenormalConst * antiDenormalSign;
        const float pitchRatio = getGlidePitchRatio();
        const float wowFlutter = getWowFlutterMultiplier();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const float dry = (float) data[sample] + antiDenormalNoise;
            float wetSum = 0.0f;

            for (int line = 0; line < numLines; ++line)
            {
                const float baseSeconds = baseMs[(size_t) line] * 0.001f;
                const float scaledSeconds = juce::jlimit (0.012f, 2.0f, userDelaySeconds * (0.42f + 0.075f * (float) line) + baseSeconds * 0.35f);
                const float delaySamples = scaledSeconds * (float) sr * wowFlutter / pitchRatio;
                lineOutputs[(size_t) line] = readTapeLine (ch, line, delaySamples);
                wetSum += lineOutputs[(size_t) line];
            }
            wetSum *= 1.0f / (float) numLines;

            for (int line = 0; line < numLines; ++line)
            {
                const float a0 = lineOutputs[0], a1 = lineOutputs[1], a2 = lineOutputs[2], a3 = lineOutputs[3];
                const float a4 = lineOutputs[4], a5 = lineOutputs[5], a6 = lineOutputs[6], a7 = lineOutputs[7];
                const float sign = 1.0f / std::sqrt (8.0f);
                float h = 0.0f;
                switch (line)
                {
                    case 0: h = ( a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7) * sign; break;
                    case 1: h = ( a0 - a1 + a2 - a3 + a4 - a5 + a6 - a7) * sign; break;
                    case 2: h = ( a0 + a1 - a2 - a3 + a4 + a5 - a6 - a7) * sign; break;
                    case 3: h = ( a0 - a1 - a2 + a3 + a4 - a5 - a6 + a7) * sign; break;
                    case 4: h = ( a0 + a1 + a2 + a3 - a4 - a5 - a6 - a7) * sign; break;
                    case 5: h = ( a0 - a1 + a2 - a3 - a4 + a5 - a6 + a7) * sign; break;
                    case 6: h = ( a0 + a1 - a2 - a3 - a4 - a5 + a6 + a7) * sign; break;
                    case 7: h = ( a0 - a1 - a2 + a3 - a4 + a5 + a6 - a7) * sign; break;
                    default: break;
                }
                feedbackVector[(size_t) line] = lineOutputs[(size_t) line] * 0.807f + h * 0.193f + antiDenormalNoise;
            }

            for (int line = 0; line < numLines; ++line)
            {
                const float channelPolarity = (ch == 0 ? 1.0f : ((line % 2 == 0) ? -1.0f : 1.0f));
                const float inputToNetwork = dry * (line == 0 || line == 3 || line == 5 ? 0.38f : 0.16f) * channelPolarity;
                const float value = inputToNetwork + feedbackVector[(size_t) line] * feedback;
                writeTapeLine (lines[(size_t) ch][(size_t) line], juce::jlimit (-2.0f, 2.0f, value));
            }

            data[sample] = (FloatType) ((float) data[sample] * (1.0f - mix) + wetSum * mix);
        }
        advanceGlidePhase();
    }
}

template void DelayGlideProcessor::process<float>  (juce::AudioBuffer<float>&);
template void DelayGlideProcessor::process<double> (juce::AudioBuffer<double>&);
