#include "Comp2AProcessor.h"

namespace
{
    static float readParam (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }

    static double scHpfFrequencyFromIndex (int idx) noexcept
    {
        switch (idx)
        {
            case 1: return 60.0;
            case 2: return 90.0;
            case 3: return 150.0;
            default: return 0.0;
        }
    }
}

void Comp2AProcessor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    peakParam     = apvts.getRawParameterValue ("comp2a_peak");
    gainParam     = apvts.getRawParameterValue ("comp2a_gain");
    modeParam     = apvts.getRawParameterValue ("comp2a_mode");
    emphasisParam = apvts.getRawParameterValue ("comp2a_emphasis");
    mixParam      = apvts.getRawParameterValue ("comp2a_mix");
    scHpfParam    = apvts.getRawParameterValue ("comp2a_sc_hpf");
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
    cellMemory.fill (0.0);
    scHpfX1.fill (0.0);
    scHpfY1.fill (0.0);
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

double Comp2AProcessor::tubeGainStage (double x, double drive) noexcept
{
    const double bias = 0.018;
    const double biased = x * (1.0 + drive * 0.30) + bias;
    const double shaped = biased / (1.0 + std::abs (biased) * 0.32);
    return (shaped - bias) * (1.0 + drive * 0.08);
}

double Comp2AProcessor::processSidechainHPF (int channel, double x) noexcept
{
    const double freq = scHpfFrequencyFromIndex (scHpfIndex);
    if (freq <= 0.0)
        return x;

    const auto idx = static_cast<size_t> (juce::jlimit (0, 7, channel));
    const double rc = 1.0 / (juce::MathConstants<double>::twoPi * freq);
    const double dt = 1.0 / juce::jmax (sr, 1.0);
    const double alpha = rc / (rc + dt);
    const double y = alpha * (scHpfY1[idx] + x - scHpfX1[idx]);
    scHpfX1[idx] = x;
    scHpfY1[idx] = y;
    return y;
}

double Comp2AProcessor::t4OptoResponseDb (int channel, double controlVoltageDb) noexcept
{
    const auto idx = static_cast<size_t> (juce::jlimit (0, 7, channel));
    const double lightIntensity = std::pow (10.0, juce::jlimit (-80.0, 36.0, controlVoltageDb) / 20.0);
    const double squared = lightIntensity * lightIntensity;
    const double dynamicTau = 0.060 + cellMemory[idx] * 5.0;
    const double coef = 1.0 - std::exp (-1.0 / (juce::jmax (dynamicTau, 0.001) * juce::jmax (sr, 1.0)));
    cellMemory[idx] += coef * (squared - cellMemory[idx]);
    cellMemory[idx] = juce::jlimit (0.0, 32.0, cellMemory[idx]);

    const double resistance = 1.0 / (0.1 + cellMemory[idx] * 4.0);
    const double grDb = -juce::Decibels::gainToDecibels (juce::jlimit (0.001, 10.0, resistance));
    return juce::jlimit (0.0, 36.0, grDb);
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
    scHpfIndex = (int) std::round (readParam (scHpfParam, 0.0f));
    setBypass (readParam (bypassParam, 1.0f) > 0.5f);

    const bool limitMode = ((int) std::round (readParam (modeParam, 0.0f))) == 1;

    for (int i = 0; i < n; ++i)
    {
        const double peak = peakSmooth.getNextValue() / 100.0;
        const double makeUp = juce::Decibels::decibelsToGain (-12.0 + gainSmooth.getNextValue() * 0.36);
        const double emphasis = emphasisSmooth.getNextValue() / 100.0;
        const double localMix = mixSmooth.getNextValue() / 100.0;
        const double wetMix = bypassWet.getNextValue();
        const double attack = smoothCoeffMs (8.0, sr);
        const double threshold = -8.0 - peak * 36.0;
        const double slope = limitMode ? 0.95 : 0.68;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            const double detectorSample = processSidechainHPF (ch, dry);
            const double detDb = peakDbFromLinear (std::abs (detectorSample) * (1.0 + emphasis * 1.5));
            const double controlVoltageDb = detDb > threshold ? (detDb - threshold) * slope : -80.0;
            const double targetGr = t4OptoResponseDb (ch, controlVoltageDb);
            const double coeff = targetGr > env[(size_t) ch] ? attack : 1.0;
            env[(size_t) ch] += coeff * (targetGr - env[(size_t) ch]);
            maxGr = juce::jmax (maxGr, env[(size_t) ch]);

            double wet = dry * juce::Decibels::decibelsToGain (-env[(size_t) ch]);
            const double drive = 0.30 + peak * 0.35 + emphasis * 0.10;
            wet = tubeGainStage (wet, drive);
            wet *= makeUp;
            const double compressed = dry * (1.0 - localMix) + wet * localMix;
            data[i] = (FloatType) (dry * (1.0 - wetMix) + compressed * wetMix);
        }
    }

    gainReductionDb.store ((float) maxGr);
}

void Comp2AProcessor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void Comp2AProcessor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
