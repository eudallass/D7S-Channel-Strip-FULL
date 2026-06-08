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
    dcCoef = 1.0 - std::exp (-juce::MathConstants<double>::twoPi * 7.0 / juce::jmax (sr, 1.0));

    for (auto* s : { &driveSmooth, &ceilingSmooth, &shapeSmooth, &mixSmooth, &bypassWet })
        s->reset (sr, 0.012);

    reset();
}

void ClipperProcessor::reset()
{
    dcX1.fill (0.0);
    dcY1.fill (0.0);
    biasPhase = 0.0;
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

double ClipperProcessor::hybridClip (double x, double shape, double bias) noexcept
{
    const double driveNorm = 1.0 + shape * 5.5;
    const double biased = x + bias;
    const double soft = std::atan (biased * driveNorm) / std::atan (driveNorm);
    const double cubic = biased - (biased * biased * biased) * 0.115;
    const double rounded = juce::jlimit (-1.15, 1.15, cubic);
    const double hard = juce::jlimit (-1.0, 1.0, rounded);
    const double out = soft * shape + hard * (1.0 - shape);
    return juce::jlimit (-1.0, 1.0, out - bias * 0.55);
}

double ClipperProcessor::processDcBlocker (int channel, double x) noexcept
{
    const auto idx = static_cast<size_t> (juce::jlimit (0, 7, channel));
    const double y = x - dcX1[idx] + (1.0 - dcCoef) * dcY1[idx];
    dcX1[idx] = x;
    dcY1[idx] = y;
    return y;
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
        const double driveDb = (double) driveSmooth.getNextValue();
        const double drive = juce::Decibels::decibelsToGain (driveDb);
        const double ceiling = juce::Decibels::decibelsToGain ((double) ceilingSmooth.getNextValue());
        const double shape = shapeSmooth.getNextValue() / 100.0;
        const double mix = mixSmooth.getNextValue() / 100.0;
        const double wetBypass = bypassWet.getNextValue();

        biasPhase += 0.17 / juce::jmax (sr, 1.0);
        if (biasPhase >= 1.0)
            biasPhase -= 1.0;
        const double bias = std::sin (biasPhase * juce::MathConstants<double>::twoPi) * 0.012 * shape;
        const double trim = juce::Decibels::decibelsToGain (-driveDb * 0.30);

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            double x = dry * drive;
            x = hybridClip (x / juce::jmax (ceiling, 1.0e-6), shape, bias) * ceiling;
            x = processDcBlocker (ch, x * trim);
            const double clipped = dry * (1.0 - mix) + x * mix;
            data[i] = (FloatType) (dry * (1.0 - wetBypass) + clipped * wetBypass);
        }
    }
}

void ClipperProcessor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void ClipperProcessor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
