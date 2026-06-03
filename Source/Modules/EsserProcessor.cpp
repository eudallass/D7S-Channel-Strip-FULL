#include "EsserProcessor.h"

namespace
{
    static float readParam (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }
}

void EsserProcessor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    thresholdParam = apvts.getRawParameterValue ("esser_threshold");
    freqParam      = apvts.getRawParameterValue ("esser_freq");
    rangeParam     = apvts.getRawParameterValue ("esser_range");
    modeParam      = apvts.getRawParameterValue ("esser_mode");
    bypassParam    = apvts.getRawParameterValue ("esser_bypass");
}

void EsserProcessor::prepare (double sampleRate, int, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, 8, numChannels);
    for (auto* s : { &thresholdSmooth, &freqSmooth, &rangeSmooth, &bypassWet })
        s->reset (sr, 0.012);
    reset();
}

void EsserProcessor::reset()
{
    lp.fill (0.0);
    env.fill (0.0);
    thresholdSmooth.setCurrentAndTargetValue (readParam (thresholdParam, -24.0f));
    freqSmooth.setCurrentAndTargetValue (readParam (freqParam, 7000.0f));
    rangeSmooth.setCurrentAndTargetValue (readParam (rangeParam, 12.0f));
    bypassWet.setCurrentAndTargetValue (readParam (bypassParam, 1.0f) > 0.5f ? 0.0f : 1.0f);
    gainReductionDb.store (0.0f);
}

void EsserProcessor::setBypass (bool shouldBypass)
{
    bypassWet.setTargetValue (shouldBypass ? 0.0f : 1.0f);
}

double EsserProcessor::onePoleCoeff (double frequency, double sampleRate) noexcept
{
    return 1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * juce::jlimit (10.0, sampleRate * 0.45, frequency) / juce::jmax (sampleRate, 1.0));
}

double EsserProcessor::smoothCoeffMs (double timeMs, double sampleRate) noexcept
{
    return 1.0 - std::exp (-1000.0 / (juce::jmax (timeMs, 0.01) * juce::jmax (sampleRate, 1.0)));
}

double EsserProcessor::peakDbFromLinear (double value) noexcept
{
    return juce::Decibels::gainToDecibels (juce::jmax (value, 1.0e-9));
}

template <typename FloatType>
void EsserProcessor::processInternal (juce::AudioBuffer<FloatType>& buffer)
{
    const int numCh = juce::jmin (channels, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    double maxGr = 0.0;

    thresholdSmooth.setTargetValue (readParam (thresholdParam, -24.0f));
    freqSmooth.setTargetValue (readParam (freqParam, 7000.0f));
    rangeSmooth.setTargetValue (readParam (rangeParam, 12.0f));
    setBypass (readParam (bypassParam, 1.0f) > 0.5f);

    const bool splitMode = ((int) std::round (readParam (modeParam, 1.0f))) == 1;
    const double attack = smoothCoeffMs (1.0, sr);
    const double release = smoothCoeffMs (90.0, sr);

    for (int i = 0; i < n; ++i)
    {
        const double threshold = thresholdSmooth.getNextValue();
        const double freq = freqSmooth.getNextValue();
        const double range = rangeSmooth.getNextValue();
        const double coeffFreq = onePoleCoeff (freq, sr);
        const double wetMix = bypassWet.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            double x = dry;
            lp[(size_t) ch] += coeffFreq * (x - lp[(size_t) ch]);
            const double sib = x - lp[(size_t) ch];
            const double level = std::abs (sib);
            const double envCoeff = level > env[(size_t) ch] ? attack : release;
            env[(size_t) ch] += envCoeff * (level - env[(size_t) ch]);
            const double over = peakDbFromLinear (env[(size_t) ch]) - threshold;
            const double reductionDb = juce::jlimit (0.0, range, over > 0.0 ? over * 0.75 : 0.0);
            maxGr = juce::jmax (maxGr, reductionDb);
            const double gr = juce::Decibels::decibelsToGain (-reductionDb);
            const double wet = splitMode ? (x - sib + sib * gr) : (x * gr);
            data[i] = (FloatType) (dry * (1.0 - wetMix) + wet * wetMix);
        }
    }

    gainReductionDb.store ((float) maxGr);
}

void EsserProcessor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void EsserProcessor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
