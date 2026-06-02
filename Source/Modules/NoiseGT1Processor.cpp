#include "NoiseGT1Processor.h"

void NoiseGT1Processor::prepare (double sr, int /*samplesPerBlock*/, int numCh)
{
    sampleRate  = sr;
    numChannels = juce::jmin (numCh, (int) signalEnvelope.size());

    signalAttackCoeff  = computeAttackCoeff  (1.0f,   sr);
    signalReleaseCoeff = computeReleaseCoeff (80.0f,  sr);

    noiseAttackCoeff  = computeAttackCoeff  (200.0f,  sr);
    noiseReleaseCoeff = computeReleaseCoeff (3000.0f, sr);

    gainAttackCoeff  = computeAttackCoeff  (5.0f,   sr);
    gainReleaseCoeff = computeReleaseCoeff (150.0f, sr);

    reset();
}

void NoiseGT1Processor::reset()
{
    signalEnvelope.fill (0.0f);
    noiseFloorEstimate.fill (1e-6f);
    currentGain = 1.0f;
    gainReductionDb.store (0.0f);
}

void NoiseGT1Processor::setSuppressionAmount (float zeroToOne)
{
    suppressionAmount = juce::jlimit (0.0f, 1.0f, zeroToOne);
}

void NoiseGT1Processor::setBypass (bool shouldBypass)
{
    bypassed = shouldBypass;
}

template <typename FloatType>
void NoiseGT1Processor::process (juce::AudioBuffer<FloatType>& buffer)
{
    if (bypassed)
    {
        gainReductionDb.store (0.0f);
        return;
    }

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float maxSignalEnv = 0.0f;
        float maxNoiseEnv  = 0.0f;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* channelData = buffer.getWritePointer (ch);
            const float absVal = std::abs ((float) channelData[sample]);

            if (absVal > signalEnvelope[ch])
                signalEnvelope[ch] += signalAttackCoeff  * (absVal - signalEnvelope[ch]);
            else
                signalEnvelope[ch] += signalReleaseCoeff * (absVal - signalEnvelope[ch]);

            const float noiseRatio = (signalEnvelope[ch] > 1e-10f)
                                     ? (noiseFloorEstimate[ch] / signalEnvelope[ch])
                                     : 1.0f;

            if (noiseRatio > 0.3f)
            {
                if (absVal > noiseFloorEstimate[ch])
                    noiseFloorEstimate[ch] += noiseAttackCoeff  * (absVal - noiseFloorEstimate[ch]);
                else
                    noiseFloorEstimate[ch] += noiseReleaseCoeff * (absVal - noiseFloorEstimate[ch]);
            }
            else
            {
                noiseFloorEstimate[ch] += noiseReleaseCoeff * (0.0f - noiseFloorEstimate[ch]);
            }

            maxSignalEnv = juce::jmax (maxSignalEnv, signalEnvelope[ch]);
            maxNoiseEnv  = juce::jmax (maxNoiseEnv,  noiseFloorEstimate[ch]);
        }

        const float gainTarget = computeGainTarget (maxSignalEnv, maxNoiseEnv);

        if (gainTarget > currentGain)
            currentGain += gainAttackCoeff  * (gainTarget - currentGain);
        else
            currentGain += gainReleaseCoeff * (gainTarget - currentGain);

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* channelData = buffer.getWritePointer (ch);
            channelData[sample] = (FloatType)(channelData[sample] * currentGain);
        }
    }

    const float grDb = (currentGain > 1e-6f)
                       ? -20.0f * std::log10 (currentGain)
                       : 60.0f;
    gainReductionDb.store (juce::jmax (0.0f, grDb));
}

template void NoiseGT1Processor::process<float>  (juce::AudioBuffer<float>&);
template void NoiseGT1Processor::process<double> (juce::AudioBuffer<double>&);

float NoiseGT1Processor::computeThreshold() const noexcept
{
    const float sqrtSupp = std::sqrt (suppressionAmount);
    const float threshDb = -90.0f + sqrtSupp * 70.0f;
    return juce::Decibels::decibelsToGain (threshDb);
}

float NoiseGT1Processor::computeGainTarget (float signalRms, float noiseRms) const noexcept
{
    if (suppressionAmount < 0.001f)
        return 1.0f;

    const float threshold = computeThreshold();
    const float noiseThresh = noiseRms * (1.0f + suppressionAmount * 8.0f);
    const float effectiveThreshold = juce::jmax (threshold, noiseThresh);

    if (signalRms >= effectiveThreshold)
        return 1.0f;

    if (signalRms < effectiveThreshold * 0.1f)
        return juce::Decibels::decibelsToGain (-60.0f);

    const float ratio = signalRms / effectiveThreshold;
    const float gainTarget = ratio * ratio;
    return juce::jlimit (juce::Decibels::decibelsToGain (-60.0f), 1.0f, gainTarget);
}

float NoiseGT1Processor::computeAttackCoeff (float timeMs, double sr) noexcept
{
    if (timeMs <= 0.0f || sr <= 0.0) return 1.0f;
    return 1.0f - std::exp (-1000.0f / (timeMs * (float) sr));
}

float NoiseGT1Processor::computeReleaseCoeff (float timeMs, double sr) noexcept
{
    if (timeMs <= 0.0f || sr <= 0.0) return 1.0f;
    return 1.0f - std::exp (-1000.0f / (timeMs * (float) sr));
}
