#include "Comp76Processor.h"

namespace
{
    static float readParam (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }
}

void Comp76Processor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    inputParam   = apvts.getRawParameterValue ("comp76_input");
    outputParam  = apvts.getRawParameterValue ("comp76_output");
    attackParam  = apvts.getRawParameterValue ("comp76_attack");
    releaseParam = apvts.getRawParameterValue ("comp76_release");
    ratioParam   = apvts.getRawParameterValue ("comp76_ratio");
    bypassParam  = apvts.getRawParameterValue ("comp76_bypass");
}

void Comp76Processor::prepare (double sampleRate, int, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, 8, numChannels);
    for (auto* s : { &inputSmooth, &outputSmooth, &attackSmooth, &releaseSmooth, &bypassWet })
        s->reset (sr, 0.015);
    reset();
}

void Comp76Processor::reset()
{
    env.fill (0.0);
    inputSmooth.setCurrentAndTargetValue (readParam (inputParam, 4.0f));
    outputSmooth.setCurrentAndTargetValue (readParam (outputParam, 5.0f));
    attackSmooth.setCurrentAndTargetValue (readParam (attackParam, 3.0f));
    releaseSmooth.setCurrentAndTargetValue (readParam (releaseParam, 5.0f));
    bypassWet.setCurrentAndTargetValue (readParam (bypassParam, 1.0f) > 0.5f ? 0.0f : 1.0f);
    gainReductionDb.store (0.0f);
}

void Comp76Processor::setBypass (bool shouldBypass)
{
    bypassWet.setTargetValue (shouldBypass ? 0.0f : 1.0f);
}

double Comp76Processor::smoothCoeffMs (double timeMs, double sampleRate) noexcept
{
    return 1.0 - std::exp (-1000.0 / (juce::jmax (timeMs, 0.01) * juce::jmax (sampleRate, 1.0)));
}

double Comp76Processor::peakDbFromLinear (double value) noexcept
{
    return juce::Decibels::gainToDecibels (juce::jmax (value, 1.0e-9));
}

template <typename FloatType>
void Comp76Processor::processInternal (juce::AudioBuffer<FloatType>& buffer)
{
    const int numCh = juce::jmin (channels, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    double maxGr = 0.0;

    inputSmooth.setTargetValue (readParam (inputParam, 4.0f));
    outputSmooth.setTargetValue (readParam (outputParam, 5.0f));
    attackSmooth.setTargetValue (readParam (attackParam, 3.0f));
    releaseSmooth.setTargetValue (readParam (releaseParam, 5.0f));
    setBypass (readParam (bypassParam, 1.0f) > 0.5f);

    const int ratioIndex = (int) std::round (readParam (ratioParam, 0.0f));
    const double ratio = ratioIndex == 0 ? 4.0 : ratioIndex == 1 ? 8.0 : ratioIndex == 2 ? 12.0 : ratioIndex == 3 ? 20.0 : 30.0;
    const double drive = ratioIndex == 4 ? 1.45 : 1.15;

    for (int i = 0; i < n; ++i)
    {
        const double inputGain = juce::Decibels::decibelsToGain (-18.0 + inputSmooth.getNextValue() * 4.2);
        const double outputGain = juce::Decibels::decibelsToGain (-18.0 + outputSmooth.getNextValue() * 3.6);
        const double attackCoeff = smoothCoeffMs (0.02 + (7.0 - attackSmooth.getNextValue()) * 1.15, sr);
        const double releaseCoeff = smoothCoeffMs (40.0 + (7.0 - releaseSmooth.getNextValue()) * 180.0, sr);
        const double wetMix = bypassWet.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            double x = dry * inputGain;
            const double det = std::abs (x);
            const double coeff = det > env[(size_t) ch] ? attackCoeff : releaseCoeff;
            env[(size_t) ch] += coeff * (det - env[(size_t) ch]);
            const double over = peakDbFromLinear (env[(size_t) ch]) - (-24.0);
            const double grDb = over > 0.0 ? over * (1.0 - (1.0 / ratio)) : 0.0;
            maxGr = juce::jmax (maxGr, grDb);
            x *= juce::Decibels::decibelsToGain (-grDb);
            x = std::tanh (x * drive) / std::tanh (drive);
            x *= outputGain;
            data[i] = (FloatType) (dry * (1.0 - wetMix) + x * wetMix);
        }
    }

    gainReductionDb.store ((float) maxGr);
}

void Comp76Processor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void Comp76Processor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
