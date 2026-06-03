#include "DelayGlideProcessor.h"
#include <cmath>

void DelayGlideProcessor::prepare (double sampleRate, int /*samplesPerBlock*/, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, maxChannels, numChannels);
    maxDelaySamples = (int) std::ceil (maxDelaySeconds * sr) + 8;

    for (auto& channelLines : lines)
    {
        for (auto& line : channelLines)
        {
            line.buffer.assign ((size_t) maxDelaySamples, 0.0f);
            line.writeIndex = 0;
        }
    }

    reset();
}

void DelayGlideProcessor::reset()
{
    for (auto& channelLines : lines)
    {
        for (auto& line : channelLines)
        {
            std::fill (line.buffer.begin(), line.buffer.end(), 0.0f);
            line.writeIndex = 0;
        }
    }

    lineOutputs.fill (0.0f);
    feedbackVector.fill (0.0f);
    glidePhase = 0.0f;
    currentRandomDirection = 1;
}

void DelayGlideProcessor::setMix (float percent) noexcept
{
    mix = juce::jlimit (0.0f, 1.0f, percent / 100.0f);
}

void DelayGlideProcessor::setFeedback (float percent) noexcept
{
    feedback = juce::jlimit (0.0f, 0.95f, percent / 100.0f * 0.95f);
}

void DelayGlideProcessor::setDelayDivision (int division) noexcept
{
    delayDivision = juce::jlimit (0, 9, division);
}

void DelayGlideProcessor::setBypass (bool shouldBypass) noexcept
{
    bypassed = shouldBypass;
}

void DelayGlideProcessor::setGlideEnabled (bool enabled) noexcept
{
    glideEnabled = enabled;
}

void DelayGlideProcessor::setGlideDirection (int direction) noexcept
{
    glideDirection = juce::jlimit (0, 2, direction);
}

void DelayGlideProcessor::setGlideTime (float percent) noexcept
{
    glideTime = juce::jlimit (0.0f, 1.0f, percent / 100.0f);
}

void DelayGlideProcessor::setTempoBpm (double bpm) noexcept
{
    tempoBpm = juce::jlimit (30.0, 300.0, bpm > 0.0 ? bpm : 120.0);
}

float DelayGlideProcessor::getDivisionBeats() const noexcept
{
    switch (delayDivision)
    {
        case div_1_32: return 0.125f;
        case div_1_16: return 0.25f;
        case div_1_8:  return 0.5f;
        case div_1_4:  return 1.0f;
        case div_1_2:  return 2.0f;
        case div_1_1:  return 4.0f;
        case div_1_8T: return 1.0f / 3.0f;
        case div_1_4T: return 2.0f / 3.0f;
        case div_1_8D: return 0.75f;
        case div_1_4D: return 1.5f;
        default: return 1.0f;
    }
}

float DelayGlideProcessor::readDelayLineLinear (DelayLine& line, float delaySamples) const noexcept
{
    const int size = (int) line.buffer.size();
    if (size <= 4)
        return 0.0f;

    delaySamples = juce::jlimit (1.0f, (float) size - 3.0f, delaySamples);
    float readPosition = (float) line.writeIndex - delaySamples;
    while (readPosition < 0.0f)
        readPosition += (float) size;

    const int index0 = (int) readPosition;
    const int index1 = (index0 + 1) % size;
    const float frac = readPosition - (float) index0;

    return line.buffer[(size_t) index0] + frac * (line.buffer[(size_t) index1] - line.buffer[(size_t) index0]);
}

void DelayGlideProcessor::writeDelayLine (DelayLine& line, float value) noexcept
{
    if (line.buffer.empty())
        return;

    line.buffer[(size_t) line.writeIndex] = value;
    line.writeIndex = (line.writeIndex + 1) % (int) line.buffer.size();
}

float DelayGlideProcessor::getGlidePitchRatio() noexcept
{
    if (! glideEnabled)
        return 1.0f;

    const float activePortion = juce::jmax (0.02f, glideTime);
    const float ramp = juce::jlimit (0.0f, 1.0f, glidePhase / activePortion);

    int dir = 1;
    if (glideDirection == glideDown)
        dir = -1;
    else if (glideDirection == glideRandom)
        dir = currentRandomDirection;

    const float semitones = 12.0f * ramp * (float) dir;
    return std::pow (2.0f, semitones / 12.0f);
}

void DelayGlideProcessor::advanceGlidePhase() noexcept
{
    const float secondsPerBeat = 60.0f / (float) tempoBpm;
    const float cycleSeconds = juce::jmax (0.02f, secondsPerBeat * getDivisionBeats());
    const float inc = 1.0f / (cycleSeconds * (float) sr);

    const float oldPhase = glidePhase;
    glidePhase += inc;

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
    if (bypassed || mix <= 0.0001f)
        return;

    const int numCh = juce::jmin (channels, buffer.getNumChannels());
    const int numSamples = buffer.getNumSamples();
    const float secondsPerBeat = 60.0f / (float) tempoBpm;
    const float userDelaySeconds = juce::jlimit (0.02f, 2.0f, secondsPerBeat * getDivisionBeats());

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float pitchRatio = getGlidePitchRatio();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const float dry = (float) data[sample];

            float wetSum = 0.0f;

            for (int line = 0; line < numLines; ++line)
            {
                auto& dl = lines[(size_t) ch][(size_t) line];

                // Gemini-like delay spread: user rhythmic time is the anchor, prime-ish line offsets create FDN diffusion.
                const float baseSeconds = baseMs[(size_t) line] * 0.001f;
                const float scaledSeconds = juce::jlimit (0.012f, 2.0f, userDelaySeconds * (0.42f + 0.075f * (float) line) + baseSeconds * 0.35f);

                // Supermassive reference config: Mod Rate 0.5 Hz, Mod Depth 50% hardcoded but kept subtle for delay clarity.
                const float lfo = std::sin (juce::MathConstants<float>::twoPi * lfoPhase[(size_t) line]);
                lfoPhase[(size_t) line] += 0.5f / (float) sr;
                if (lfoPhase[(size_t) line] >= 1.0f)
                    lfoPhase[(size_t) line] -= 1.0f;

                const float modulation = 1.0f + lfo * 0.0125f;
                const float delaySamples = scaledSeconds * (float) sr * modulation / pitchRatio;

                lineOutputs[(size_t) line] = readDelayLineLinear (dl, delaySamples);
                wetSum += lineOutputs[(size_t) line];
            }

            wetSum *= 1.0f / (float) numLines;

            // 8x8 Hadamard-style orthogonal feedback matrix. This preserves energy and creates wide FDN diffusion.
            // Density is intentionally low (~19%) by mixing mostly diagonal/self energy with a controlled Hadamard crossfeed.
            for (int line = 0; line < numLines; ++line)
            {
                const float a0 = lineOutputs[0];
                const float a1 = lineOutputs[1];
                const float a2 = lineOutputs[2];
                const float a3 = lineOutputs[3];
                const float a4 = lineOutputs[4];
                const float a5 = lineOutputs[5];
                const float a6 = lineOutputs[6];
                const float a7 = lineOutputs[7];

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

                const float lowDensityCrossfeed = 0.193f;
                feedbackVector[(size_t) line] = lineOutputs[(size_t) line] * (1.0f - lowDensityCrossfeed) + h * lowDensityCrossfeed;
            }

            for (int line = 0; line < numLines; ++line)
            {
                // Small channel offset supports width 100% without needing a visible width control.
                const float channelPolarity = (ch == 0 ? 1.0f : ((line % 2 == 0) ? -1.0f : 1.0f));
                const float inputToNetwork = dry * (line == 0 || line == 3 || line == 5 ? 0.38f : 0.16f) * channelPolarity;
                const float value = inputToNetwork + feedbackVector[(size_t) line] * feedback;
                writeDelayLine (lines[(size_t) ch][(size_t) line], juce::jlimit (-2.0f, 2.0f, value));
            }

            data[sample] = (FloatType) (dry * (1.0f - mix) + wetSum * mix);
        }

        advanceGlidePhase();
    }
}

template void DelayGlideProcessor::process<float>  (juce::AudioBuffer<float>&);
template void DelayGlideProcessor::process<double> (juce::AudioBuffer<double>&);
