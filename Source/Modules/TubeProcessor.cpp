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
    dcBlockerCoef = 1.0 - std::exp (-juce::MathConstants<double>::twoPi * 5.0 / juce::jmax (sr, 1.0));

    for (auto* s : { &beautySmooth, &beastSmooth, &sensitivitySmooth, &mixSmooth, &bypassWet })
        s->reset (sr, 0.015);
    reset();
}

void TubeProcessor::reset()
{
    beautyState.fill (0.0);
    beastState.fill (0.0);
    dcX1.fill (0.0);
    dcY1.fill (0.0);
    biasDriftPhase = 0.0;
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

double TubeProcessor::tubeWaveshape (double x, double bias, double drive) noexcept
{
    const double biased = x * drive + bias;
    const double shaped = biased / (1.0 + std::abs (biased) * 0.7);
    return shaped - bias;
}

double TubeProcessor::nextBiasDrift() noexcept
{
    constexpr double lfoRateHz = 0.15;
    biasDriftPhase += lfoRateHz / juce::jmax (sr, 1.0);
    if (biasDriftPhase >= 1.0)
        biasDriftPhase -= 1.0;

    const double lfo = std::sin (biasDriftPhase * juce::MathConstants<double>::twoPi);
    const double jitter = ((double) biasJitterRng.nextFloat() - 0.5) * 0.4;
    return (lfo + jitter * 0.3) * 0.05;
}

double TubeProcessor::processDcBlocker (int channel, double x) noexcept
{
    const auto idx = static_cast<size_t> (juce::jlimit (0, 7, channel));
    const double y = x - dcX1[idx] + (1.0 - dcBlockerCoef) * dcY1[idx];
    dcX1[idx] = x;
    dcY1[idx] = y;
    return y;
}

template <typename FloatType>
void TubeProcessor::processInternal (juce::AudioBuffer<FloatType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

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
        const double drift = nextBiasDrift();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            const double driven = dry * sens;
            beautyState[(size_t) ch] += beautyCoeff * (driven - beautyState[(size_t) ch]);
            beastState[(size_t) ch] += beastCoeff * (driven - beastState[(size_t) ch]);

            const double highBand = driven - beautyState[(size_t) ch];
            const double lowBand = beastState[(size_t) ch];
            const double beautyBias = 0.035 * (1.0 + drift);
            const double beastBias = -0.055 * (1.0 + drift);
            const double beautyWet = tubeWaveshape (highBand, beautyBias, 1.0 + beauty * 5.5);
            const double beastWet = tubeWaveshape (lowBand, beastBias, 1.0 + beast * 13.0);

            double wet = driven + beautyWet * beauty * 0.48 + beastWet * beast * 0.70;
            wet = processDcBlocker (ch, wet);
            wet *= juce::Decibels::decibelsToGain (-(beauty * 1.5 + beast * 3.0));
            const double mixed = dry * (1.0 - localMix) + wet * localMix;
            data[i] = (FloatType) (dry * (1.0 - wetMix) + mixed * wetMix);
        }
    }
}

void TubeProcessor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void TubeProcessor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
