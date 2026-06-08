#include "ClipperProcessor.h"

namespace
{
    static float readParam (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }
}

void ClipperProcessor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    driveParam   = apvts.getRawParameterValue ("clipper_drive");
    ceilingParam = apvts.getRawParameterValue ("clipper_ceiling");
    shapeParam   = apvts.getRawParameterValue ("clipper_shape");
    mixParam     = apvts.getRawParameterValue ("clipper_mix");
    bypassParam  = apvts.getRawParameterValue ("clipper_bypass");
}

void ClipperProcessor::prepare (double sampleRate, int, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, 8, numChannels);

    for (auto* s : { &driveSmooth, &ceilingSmooth, &shapeSmooth, &mixSmooth, &bypassWet })
        s->reset (sr, 0.012);

    reset();
}

void ClipperProcessor::reset()
{
    driveSmooth.setCurrentAndTargetValue (readParam (driveParam, 0.0f));
    ceilingSmooth.setCurrentAndTargetValue (readParam (ceilingParam, -0.1f));
    shapeSmooth.setCurrentAndTargetValue (readParam (shapeParam, 50.0f));
    mixSmooth.setCurrentAndTargetValue (readParam (mixParam, 100.0f));
    bypassWet.setCurrentAndTargetValue (readParam (bypassParam, 1.0f) > 0.5f ? 0.0f : 1.0f);
}

void ClipperProcessor::setBypass (bool shouldBypass)
{
    bypassWet.setTargetValue (shouldBypass ? 0.0f : 1.0f);
}

double ClipperProcessor::softClip (double x, double shape) noexcept
{
    const double hard = juce::jlimit (-1.0, 1.0, x);
    const double soft = std::tanh (x * (1.0 + shape * 3.0)) / std::tanh (1.0 + shape * 3.0);
    return soft * shape + hard * (1.0 - shape);
}

template <typename FloatType>
void ClipperProcessor::processInternal (juce::AudioBuffer<FloatType>& buffer)
{
    const int numCh = juce::jmin (channels, buffer.getNumChannels());
    const int n = buffer.getNumSamples();

    driveSmooth.setTargetValue (readParam (driveParam, 0.0f));
    ceilingSmooth.setTargetValue (readParam (ceilingParam, -0.1f));
    shapeSmooth.setTargetValue (readParam (shapeParam, 50.0f));
    mixSmooth.setTargetValue (readParam (mixParam, 100.0f));
    setBypass (readParam (bypassParam, 1.0f) > 0.5f);

    for (int i = 0; i < n; ++i)
    {
        const double drive = juce::Decibels::decibelsToGain ((double) driveSmooth.getNextValue());
        const double ceiling = juce::Decibels::decibelsToGain ((double) ceilingSmooth.getNextValue());
        const double shape = shapeSmooth.getNextValue() / 100.0;
        const double mix = mixSmooth.getNextValue() / 100.0;
        const double wetBypass = bypassWet.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            double x = dry * drive;
            x = softClip (x / juce::jmax (ceiling, 1.0e-6), shape) * ceiling;
            const double clipped = dry * (1.0 - mix) + x * mix;
            data[i] = (FloatType) (dry * (1.0 - wetBypass) + clipped * wetBypass);
        }
    }
}

void ClipperProcessor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void ClipperProcessor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
