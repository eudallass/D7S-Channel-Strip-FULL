#include "EQ4KProcessor.h"

namespace
{
    static float readParam (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }
}

void EQ4KProcessor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    hpfParam     = apvts.getRawParameterValue ("eq4k_hpf");
    lpfParam     = apvts.getRawParameterValue ("eq4k_lpf");
    lfFreqParam  = apvts.getRawParameterValue ("eq4k_lf_freq");
    lfGainParam  = apvts.getRawParameterValue ("eq4k_lf_gain");
    lfBellParam  = apvts.getRawParameterValue ("eq4k_lf_bell");
    lmfFreqParam = apvts.getRawParameterValue ("eq4k_lmf_freq");
    lmfGainParam = apvts.getRawParameterValue ("eq4k_lmf_gain");
    lmfQParam    = apvts.getRawParameterValue ("eq4k_lmf_q");
    hmfFreqParam = apvts.getRawParameterValue ("eq4k_hmf_freq");
    hmfGainParam = apvts.getRawParameterValue ("eq4k_hmf_gain");
    hmfQParam    = apvts.getRawParameterValue ("eq4k_hmf_q");
    hfFreqParam  = apvts.getRawParameterValue ("eq4k_hf_freq");
    hfGainParam  = apvts.getRawParameterValue ("eq4k_hf_gain");
    hfBellParam  = apvts.getRawParameterValue ("eq4k_hf_bell");
    bypassParam  = apvts.getRawParameterValue ("eq4k_bypass");
}

void EQ4KProcessor::prepare (double sampleRate, int, int numChannels)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channels = juce::jlimit (1, 8, numChannels);

    for (auto* s : { &hpfSmooth, &lpfSmooth, &lfFreqSmooth, &lfGainSmooth, &lmfFreqSmooth, &lmfGainSmooth, &lmfQSmooth,
                     &hmfFreqSmooth, &hmfGainSmooth, &hmfQSmooth, &hfFreqSmooth, &hfGainSmooth, &bypassWet })
        s->reset (sr, 0.015);

    reset();
}

void EQ4KProcessor::reset()
{
    hpfState.fill (0.0); lpfState.fill (0.0); lfState.fill (0.0);
    lmfZ1.fill (0.0); lmfZ2.fill (0.0); hmfZ1.fill (0.0); hmfZ2.fill (0.0); hfState.fill (0.0);

    hpfSmooth.setCurrentAndTargetValue (readParam (hpfParam, 20.0f));
    lpfSmooth.setCurrentAndTargetValue (readParam (lpfParam, 22000.0f));
    lfFreqSmooth.setCurrentAndTargetValue (readParam (lfFreqParam, 80.0f));
    lfGainSmooth.setCurrentAndTargetValue (readParam (lfGainParam, 0.0f));
    lmfFreqSmooth.setCurrentAndTargetValue (readParam (lmfFreqParam, 600.0f));
    lmfGainSmooth.setCurrentAndTargetValue (readParam (lmfGainParam, 0.0f));
    lmfQSmooth.setCurrentAndTargetValue (readParam (lmfQParam, 1.0f));
    hmfFreqSmooth.setCurrentAndTargetValue (readParam (hmfFreqParam, 3000.0f));
    hmfGainSmooth.setCurrentAndTargetValue (readParam (hmfGainParam, 0.0f));
    hmfQSmooth.setCurrentAndTargetValue (readParam (hmfQParam, 1.0f));
    hfFreqSmooth.setCurrentAndTargetValue (readParam (hfFreqParam, 10000.0f));
    hfGainSmooth.setCurrentAndTargetValue (readParam (hfGainParam, 0.0f));
    bypassWet.setCurrentAndTargetValue (readParam (bypassParam, 1.0f) > 0.5f ? 0.0f : 1.0f);
}

void EQ4KProcessor::setBypass (bool shouldBypass)
{
    bypassWet.setTargetValue (shouldBypass ? 0.0f : 1.0f);
}

double EQ4KProcessor::onePoleCoeff (double frequency, double sampleRate) noexcept
{
    return 1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * juce::jlimit (10.0, sampleRate * 0.45, frequency) / juce::jmax (sampleRate, 1.0));
}

EQ4KProcessor::BiquadCoefficients EQ4KProcessor::makePeakingEQ (double frequency, double q, double gainDb, double sampleRate) noexcept
{
    BiquadCoefficients c;
    frequency = juce::jlimit (20.0, sampleRate * 0.45, frequency);
    q = juce::jlimit (0.25, 8.0, q);
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double cosw0 = std::cos (w0);
    const double b0 = 1.0 + alpha * A;
    const double b1 = -2.0 * cosw0;
    const double b2 = 1.0 - alpha * A;
    const double a0 = 1.0 + alpha / A;
    const double a1 = -2.0 * cosw0;
    const double a2 = 1.0 - alpha / A;
    c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0; c.a1 = a1 / a0; c.a2 = a2 / a0;
    return c;
}

double EQ4KProcessor::processBiquad (double x, const BiquadCoefficients& c, double& z1, double& z2) noexcept
{
    const double y = c.b0 * x + z1;
    z1 = c.b1 * x - c.a1 * y + z2;
    z2 = c.b2 * x - c.a2 * y;
    return y;
}

template <typename FloatType>
void EQ4KProcessor::processInternal (juce::AudioBuffer<FloatType>& buffer)
{
    const int numCh = juce::jmin (channels, buffer.getNumChannels());
    const int n = buffer.getNumSamples();

    hpfSmooth.setTargetValue (readParam (hpfParam, 20.0f));
    lpfSmooth.setTargetValue (readParam (lpfParam, 22000.0f));
    lfFreqSmooth.setTargetValue (readParam (lfFreqParam, 80.0f));
    lfGainSmooth.setTargetValue (readParam (lfGainParam, 0.0f));
    lmfFreqSmooth.setTargetValue (readParam (lmfFreqParam, 600.0f));
    lmfGainSmooth.setTargetValue (readParam (lmfGainParam, 0.0f));
    lmfQSmooth.setTargetValue (readParam (lmfQParam, 1.0f));
    hmfFreqSmooth.setTargetValue (readParam (hmfFreqParam, 3000.0f));
    hmfGainSmooth.setTargetValue (readParam (hmfGainParam, 0.0f));
    hmfQSmooth.setTargetValue (readParam (hmfQParam, 1.0f));
    hfFreqSmooth.setTargetValue (readParam (hfFreqParam, 10000.0f));
    hfGainSmooth.setTargetValue (readParam (hfGainParam, 0.0f));
    setBypass (readParam (bypassParam, 1.0f) > 0.5f);

    for (int i = 0; i < n; ++i)
    {
        const double hpf = hpfSmooth.getNextValue();
        const double lpf = lpfSmooth.getNextValue();
        const double lfFreq = lfFreqSmooth.getNextValue();
        const double lfGain = juce::Decibels::decibelsToGain ((double) lfGainSmooth.getNextValue());
        const auto lmf = makePeakingEQ (lmfFreqSmooth.getNextValue(), lmfQSmooth.getNextValue(), lmfGainSmooth.getNextValue(), sr);
        const auto hmf = makePeakingEQ (hmfFreqSmooth.getNextValue(), hmfQSmooth.getNextValue(), hmfGainSmooth.getNextValue(), sr);
        const double hfFreq = hfFreqSmooth.getNextValue();
        const double hfGain = juce::Decibels::decibelsToGain ((double) hfGainSmooth.getNextValue());
        const double wetMix = bypassWet.getNextValue();
        const double hpfC = onePoleCoeff (hpf, sr);
        const double lpfC = onePoleCoeff (lpf, sr);
        const double lfC = onePoleCoeff (lfFreq, sr);
        const double hfC = onePoleCoeff (hfFreq, sr);

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            double x = dry;
            hpfState[(size_t) ch] += hpfC * (x - hpfState[(size_t) ch]); x -= hpfState[(size_t) ch];
            lpfState[(size_t) ch] += lpfC * (x - lpfState[(size_t) ch]); x = lpfState[(size_t) ch];
            lfState[(size_t) ch] += lfC * (x - lfState[(size_t) ch]); x += lfState[(size_t) ch] * (lfGain - 1.0);
            x = processBiquad (x, lmf, lmfZ1[(size_t) ch], lmfZ2[(size_t) ch]);
            x = processBiquad (x, hmf, hmfZ1[(size_t) ch], hmfZ2[(size_t) ch]);
            hfState[(size_t) ch] += hfC * (x - hfState[(size_t) ch]); x += (x - hfState[(size_t) ch]) * (hfGain - 1.0);
            x = std::tanh (x * 1.015) / std::tanh (1.015);
            data[i] = (FloatType) (dry * (1.0 - wetMix) + x * wetMix);
        }
    }
}

void EQ4KProcessor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void EQ4KProcessor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }
