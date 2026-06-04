#include "Comp76Processor.h"

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

void Comp76Processor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    inputParam   = apvts.getRawParameterValue ("comp76_input");
    outputParam  = apvts.getRawParameterValue ("comp76_output");
    attackParam  = apvts.getRawParameterValue ("comp76_attack");
    releaseParam = apvts.getRawParameterValue ("comp76_release");
    ratioParam   = apvts.getRawParameterValue ("comp76_ratio");
    scHpfParam   = apvts.getRawParameterValue ("comp76_sc_hpf");
    bypassParam  = apvts.getRawParameterValue ("comp76_bypass");
}

void Comp76Processor::prepare (double sampleRate, int, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, 8, numChannels);
    lookaheadSamples = static_cast<int> (0.003 * sr);
    longTermCoef = 1.0 - std::exp (-1.0 / (0.300 * sr));

    for (auto* s : { &inputSmooth, &outputSmooth, &attackSmooth, &releaseSmooth, &bypassWet })
        s->reset (sr, 0.015);

    reset();
}

void Comp76Processor::reset()
{
    env.fill (-120.0);
    longTermGrAvg.fill (0.0);
    scHpfX1.fill (0.0);
    scHpfY1.fill (0.0);
    allButtonsPhaseA = 0.0;
    allButtonsPhaseB = 0.37;
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

double Comp76Processor::processSidechainHPF (int channel, double x) noexcept
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

double Comp76Processor::dynamicReleaseCoeff (int channel, double baseReleaseMs, double currentGrDb) noexcept
{
    const auto idx = static_cast<size_t> (juce::jlimit (0, 7, channel));
    longTermGrAvg[idx] += longTermCoef * (currentGrDb - longTermGrAvg[idx]);

    const double multiplier = 1.0 + (longTermGrAvg[idx] / 6.0) * 2.5;
    const double adjustedReleaseMs = baseReleaseMs * juce::jlimit (1.0, 4.0, multiplier);
    return smoothCoeffMs (adjustedReleaseMs, sr);
}

double Comp76Processor::getAllButtonsBiasDrift() noexcept
{
    allButtonsPhaseA += 0.071 / juce::jmax (sr, 1.0);
    allButtonsPhaseB += 0.113 / juce::jmax (sr, 1.0);
    if (allButtonsPhaseA >= 1.0) allButtonsPhaseA -= 1.0;
    if (allButtonsPhaseB >= 1.0) allButtonsPhaseB -= 1.0;

    const double a = std::sin (allButtonsPhaseA * juce::MathConstants<double>::twoPi);
    const double b = std::sin (allButtonsPhaseB * juce::MathConstants<double>::twoPi + 1.9);
    return 0.05 * (0.65 * a + 0.35 * b);
}

double Comp76Processor::computeAllButtonsGainReductionDb (double envDb) noexcept
{
    constexpr double threshold = -24.0;
    if (envDb <= threshold)
        return 0.0;

    const double over = envDb - threshold;
    double grDb = over * 0.985;

    if (over > 6.0)
    {
        const double chaos = std::sin (over * 0.91) * 0.75 + std::sin (over * 0.37 + 1.7) * 0.45;
        grDb += (over - 6.0) * (0.18 + 0.06 * chaos);
    }

    if (over > 14.0)
        grDb -= std::sin ((over - 14.0) * 0.63) * 1.3;

    return juce::jlimit (0.0, 44.0, grDb);
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
    scHpfIndex = (int) std::round (readParam (scHpfParam, 0.0f));
    setBypass (readParam (bypassParam, 1.0f) > 0.5f);

    const int ratioIndex = (int) std::round (readParam (ratioParam, 0.0f));
    const bool allButtons = ratioIndex == 4;
    const double ratio = ratioIndex == 0 ? 4.0 : ratioIndex == 1 ? 8.0 : ratioIndex == 2 ? 12.0 : ratioIndex == 3 ? 20.0 : 30.0;
    const double drive = allButtons ? 1.68 : 1.15;

    for (int i = 0; i < n; ++i)
    {
        const double inputGain = juce::Decibels::decibelsToGain (-18.0 + inputSmooth.getNextValue() * 4.2);
        const double outputGain = juce::Decibels::decibelsToGain (-18.0 + outputSmooth.getNextValue() * 3.6);
        const double attackCoeff = smoothCoeffMs (0.02 + (7.0 - attackSmooth.getNextValue()) * 1.15, sr);
        const double baseReleaseMs = 40.0 + (7.0 - releaseSmooth.getNextValue()) * 95.0;
        const double wetMix = bypassWet.getNextValue();
        const double biasDrift = allButtons ? getAllButtonsBiasDrift() : 0.0;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            double x = dry * inputGain;
            const double detectorSample = processSidechainHPF (ch, x);
            const double detDb = peakDbFromLinear (std::abs (detectorSample));
            const double over = env[(size_t) ch] - (-24.0);
            const double predictedGrDb = allButtons ? computeAllButtonsGainReductionDb (env[(size_t) ch])
                                                     : (over > 0.0 ? over * (1.0 - (1.0 / ratio)) : 0.0);
            const double releaseCoeff = dynamicReleaseCoeff (ch, baseReleaseMs, predictedGrDb);
            const double coeff = detDb > env[(size_t) ch] ? attackCoeff : releaseCoeff;
            env[(size_t) ch] += coeff * (detDb - env[(size_t) ch]);

            const double currentOver = env[(size_t) ch] - (-24.0);
            const double grDb = allButtons ? computeAllButtonsGainReductionDb (env[(size_t) ch])
                                           : (currentOver > 0.0 ? currentOver * (1.0 - (1.0 / ratio)) : 0.0);
            maxGr = juce::jmax (maxGr, grDb);
            x *= juce::Decibels::decibelsToGain (-grDb);

            if (allButtons)
            {
                const double asym = x + biasDrift;
                x = std::tanh (asym * drive) / std::tanh (drive) - biasDrift * 0.38;
                x += 0.035 * std::sin (x * 9.0 + biasDrift * 11.0) * juce::jlimit (0.0, 1.0, grDb / 18.0);
            }
            else
            {
                x = std::tanh (x * drive) / std::tanh (drive);
            }

            x *= outputGain;
            data[i] = (FloatType) (dry * (1.0 - wetMix) + x * wetMix);
        }
    }

    gainReductionDb.store ((float) maxGr);
}

void Comp76Processor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void Comp76Processor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
