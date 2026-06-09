#include "ClipperProcessor.h"

namespace
{
    static float readParam (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }

    static float dbToGain (float dB) noexcept
    {
        return juce::Decibels::decibelsToGain (dB);
    }
}

void ClipperProcessor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    thresholdParam = apvts.getRawParameterValue ("clipper_threshold");
    preParam       = apvts.getRawParameterValue ("clipper_pre");
    postParam      = apvts.getRawParameterValue ("clipper_post");
    bypassParam    = apvts.getRawParameterValue ("clipper_bypass");
}

void ClipperProcessor::prepare (double sampleRate, int, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, 8, numChannels);
    dcCoef = 1.0f - std::exp ((float) (-juce::MathConstants<double>::twoPi * 7.0 / juce::jmax (sr, 1.0)));

    for (auto* s : { &thresholdLinearSmooth, &preGainLinearSmooth, &postGainLinearSmooth, &bypassWet })
        s->reset (sr, 0.015);

    reset();
}

void ClipperProcessor::reset()
{
    dcX1.fill (0.0f);
    dcY1.fill (0.0f);
    thresholdLinearSmooth.setCurrentAndTargetValue (dbToGain (readParam (thresholdParam, -4.4f)));
    preGainLinearSmooth.setCurrentAndTargetValue (dbToGain (readParam (preParam, 0.0f)));
    postGainLinearSmooth.setCurrentAndTargetValue (dbToGain (readParam (postParam, 0.0f)));
    bypassWet.setCurrentAndTargetValue (readParam (bypassParam, 0.0f) > 0.5f ? 0.0f : 1.0f);
}

void ClipperProcessor::setBypass (bool shouldBypass)
{
    bypassWet.setTargetValue (shouldBypass ? 0.0f : 1.0f);
}

float ClipperProcessor::fsClip (float x, float thresholdLinear) noexcept
{
    constexpr float ceiling = 1.0f;
    const float t = juce::jlimit (0.0001f, 0.9999f, thresholdLinear);
    const float range = ceiling - t;

    if (x > t)
        return t + range * std::tanh ((x - t) / range);

    if (x < -t)
        return -t + range * std::tanh ((x + t) / range);

    return x;
}

float ClipperProcessor::processDcBlocker (int channel, float x) noexcept
{
    const auto idx = static_cast<size_t> (juce::jlimit (0, 7, channel));
    const float y = x - dcX1[idx] + (1.0f - dcCoef) * dcY1[idx];
    dcX1[idx] = x;
    dcY1[idx] = y;
    return y;
}

template <typename FloatType>
void ClipperProcessor::processInternal (juce::AudioBuffer<FloatType>& buffer) noexcept
{
    juce::ScopedNoDenormals scopedNoDenormals;

    const int numCh = juce::jmin (channels, buffer.getNumChannels());
    const int numSamples = buffer.getNumSamples();

    thresholdLinearSmooth.setTargetValue (dbToGain (readParam (thresholdParam, -4.4f)));
    preGainLinearSmooth.setTargetValue (dbToGain (readParam (preParam, 0.0f)));
    postGainLinearSmooth.setTargetValue (dbToGain (readParam (postParam, 0.0f)));
    setBypass (readParam (bypassParam, 0.0f) > 0.5f);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float thresholdLin = thresholdLinearSmooth.getNextValue();
        const float preGain = preGainLinearSmooth.getNextValue();
        const float postGain = postGainLinearSmooth.getNextValue();
        const float wetBypass = bypassWet.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const float dry = static_cast<float> (data[sample]);

            float wet = dry * preGain;
            wet = fsClip (wet, thresholdLin);
            wet = processDcBlocker (ch, wet);
            wet *= postGain;

            data[sample] = static_cast<FloatType> (dry * (1.0f - wetBypass) + wet * wetBypass);
        }
    }
}

void ClipperProcessor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void ClipperProcessor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
