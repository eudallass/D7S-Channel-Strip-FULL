#include "TubeProcessor.h"

namespace
{
    static float readParam (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }
}

void TubeProcessor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    beautyParam      = apvts.getRawParameterValue ("tube_beauty");
    beastParam       = apvts.getRawParameterValue ("tube_beast");
    sensitivityParam = apvts.getRawParameterValue ("tube_sensitivity");
    mixParam         = apvts.getRawParameterValue ("tube_mix");
    bypassParam      = apvts.getRawParameterValue ("tube_bypass");
}

void TubeProcessor::prepare (double sampleRate, int, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, 8, numChannels);
    for (auto* s : { &beautySmooth, &beastSmooth, &sensitivitySmooth, &mixSmooth, &bypassWet })
        s->reset (sr, 0.015);
    reset();
}

void TubeProcessor::reset()
{
    beautyState.fill (0.0);
    beastState.fill (0.0);
    beautySmooth.setCurrentAndTargetValue (readParam (beautyParam, 0.0f));
    beastSmooth.setCurrentAndTargetValue (readParam (beastParam, 0.0f));
    sensitivitySmooth.setCurrentAndTargetValue (readParam (sensitivityParam, 50.0f));
    mixSmooth.setCurrentAndTargetValue (readParam (mixParam, 100.0f));
    bypassWet.setCurrentAndTargetValue (readParam (bypassParam, 1.0f) > 0.5f ? 0.0f : 1.0f);
}

void TubeProcessor::setBypass (bool shouldBypass)
{
    bypassWet.setTargetValue (shouldBypass ? 0.0f : 1.0f);
}

double TubeProcessor::onePoleCoeff (double frequency, double sampleRate) noexcept
{
    return 1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * juce::jlimit (10.0, sampleRate * 0.45, frequency) / juce::jmax (sampleRate, 1.0));
}

template <typename FloatType>
void TubeProcessor::processInternal (juce::AudioBuffer<FloatType>& buffer)
{
    const int numCh = juce::jmin (channels, buffer.getNumChannels());
    const int n = buffer.getNumSamples();

    beautySmooth.setTargetValue (readParam (beautyParam, 0.0f));
    beastSmooth.setTargetValue (readParam (beastParam, 0.0f));
    sensitivitySmooth.setTargetValue (readParam (sensitivityParam, 50.0f));
    mixSmooth.setTargetValue (readParam (mixParam, 100.0f));
    setBypass (readParam (bypassParam, 1.0f) > 0.5f);

    const double beautyCoeff = onePoleCoeff (2500.0, sr);
    const double beastCoeff = onePoleCoeff (220.0, sr);

    for (int i = 0; i < n; ++i)
    {
        const double beauty = beautySmooth.getNextValue() / 100.0;
        const double beast = beastSmooth.getNextValue() / 100.0;
        const double sens = juce::Decibels::decibelsToGain (-12.0 + sensitivitySmooth.getNextValue() * 0.24);
        const double localMix = mixSmooth.getNextValue() / 100.0;
        const double wetMix = bypassWet.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            const double driven = dry * sens;
            beautyState[(size_t) ch] += beautyCoeff * (driven - beautyState[(size_t) ch]);
            beastState[(size_t) ch] += beastCoeff * (driven - beastState[(size_t) ch]);
            const double beautyWet = std::tanh ((driven - beautyState[(size_t) ch]) * (1.0 + beauty * 5.0));
            const double beastWet = std::tanh (beastState[(size_t) ch] * (1.0 + beast * 12.0));
            double wet = driven + beautyWet * beauty * 0.45 + beastWet * beast * 0.65;
            wet *= juce::Decibels::decibelsToGain (-(beauty * 1.5 + beast * 3.0));
            const double mixed = dry * (1.0 - localMix) + wet * localMix;
            data[i] = (FloatType) (dry * (1.0 - wetMix) + mixed * wetMix);
        }
    }
}

void TubeProcessor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void TubeProcessor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
