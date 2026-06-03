#include "Comp2AProcessor.h"

namespace
{
    static float readParam (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }
}

void Comp2AProcessor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    peakParam     = apvts.getRawParameterValue ("comp2a_peak");
    gainParam     = apvts.getRawParameterValue ("comp2a_gain");
    modeParam     = apvts.getRawParameterValue ("comp2a_mode");
    emphasisParam = apvts.getRawParameterValue ("comp2a_emphasis");
    mixParam      = apvts.getRawParameterValue ("comp2a_mix");
    bypassParam   = apvts.getRawParameterValue ("comp2a_bypass");
}

void Comp2AProcessor::prepare (double sampleRate, int, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, 8, numChannels);
    for (auto* s : { &peakSmooth, &gainSmooth, &emphasisSmooth, &mixSmooth, &bypassWet })
        s->reset (sr, 0.018);
    reset();
}

void Comp2AProcessor::reset()
{
    env.fill (0.0);
    peakSmooth.setCurrentAndTargetValue (readParam (peakParam, 0.0f));
    gainSmooth.setCurrentAndTargetValue (readParam (gainParam, 40.0f));
    emphasisSmooth.setCurrentAndTargetValue (readParam (emphasisParam, 0.0f));
    mixSmooth.setCurrentAndTargetValue (readParam (mixParam, 100.0f));
    bypassWet.setCurrentAndTargetValue (readParam (bypassParam, 1.0f) > 0.5f ? 0.0f : 1.0f);
    gainReductionDb.store (0.0f);
}

void Comp2AProcessor::setBypass (bool shouldBypass)
{
    bypassWet.setTargetValue (shouldBypass ? 0.0f : 1.0f);
}

double Comp2AProcessor::smoothCoeffMs (double timeMs, double sampleRate) noexcept
{
    return 1.0 - std::exp (-1000.0 / (juce::jmax (timeMs, 0.01) * juce::jmax (sampleRate, 1.0)));
}

double Comp2AProcessor::peakDbFromLinear (double value) noexcept
{
    return juce::Decibels::gainToDecibels (juce::jmax (value, 1.0e-9));
}

template <typename FloatType>
void Comp2AProcessor::processInternal (juce::AudioBuffer<FloatType>& buffer)
{
    const int numCh = juce::jmin (channels, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    double maxGr = 0.0;

    peakSmooth.setTargetValue (readParam (peakParam, 0.0f));
    gainSmooth.setTargetValue (readParam (gainParam, 40.0f));
    emphasisSmooth.setTargetValue (readParam (emphasisParam, 0.0f));
    mixSmooth.setTargetValue (readParam (mixParam, 100.0f));
    setBypass (readParam (bypassParam, 1.0f) > 0.5f);

    const bool limitMode = ((int) std::round (readParam (modeParam, 0.0f))) == 1;

    for (int i = 0; i < n; ++i)
    {
        const double peak = peakSmooth.getNextValue() / 100.0;
        const double makeUp = juce::Decibels::decibelsToGain (-12.0 + gainSmooth.getNextValue() * 0.36);
        const double emphasis = emphasisSmooth.getNextValue() / 100.0;
        const double localMix = mixSmooth.getNextValue() / 100.0;
        const double wetMix = bypassWet.getNextValue();
        const double attack = smoothCoeffMs (10.0, sr);
        const double release = smoothCoeffMs (700.0, sr);
        const double threshold = -8.0 - peak * 36.0;
        const double slope = limitMode ? 0.78 : 0.52;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            const double detDb = peakDbFromLinear (std::abs (dry) * (1.0 + emphasis * 1.5));
            const double target = detDb > threshold ? (detDb - threshold) * slope : 0.0;
            const double coeff = target > env[(size_t) ch] ? attack : release;
            env[(size_t) ch] += coeff * (target - env[(size_t) ch]);
            maxGr = juce::jmax (maxGr, env[(size_t) ch]);
            double wet = dry * juce::Decibels::decibelsToGain (-env[(size_t) ch]);
            wet = std::tanh (wet * 1.05) / std::tanh (1.05);
            wet *= makeUp;
            const double compressed = dry * (1.0 - localMix) + wet * localMix;
            data[i] = (FloatType) (dry * (1.0 - wetMix) + compressed * wetMix);
        }
    }

    gainReductionDb.store ((float) maxGr);
}

void Comp2AProcessor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void Comp2AProcessor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
