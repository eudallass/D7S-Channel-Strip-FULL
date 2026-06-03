#include "NoiseGT1Processor.h"

namespace
{
    static float readParam (std::atomic<float>* p, float fallback) noexcept
    {
        return p != nullptr ? p->load() : fallback;
    }
}

void NoiseGT1Processor::cacheParameters (juce::AudioProcessorValueTreeState& apvts)
{
    suppressionParam = apvts.getRawParameterValue ("noisegt1_suppression");
    bypassParam = apvts.getRawParameterValue ("noisegt1_bypass");
}

void NoiseGT1Processor::prepare (double sr, int /*samplesPerBlock*/, int numCh)
{
    sampleRate  = sr;
    numChannels = juce::jmin (numCh, (int) signalEnvelope.size());

    signalAttackCoeff  = computeAttackCoeff  (0.5f,  sr);
    signalReleaseCoeff = computeReleaseCoeff (45.0f, sr);

    noiseAttackCoeff  = computeAttackCoeff  (80.0f,   sr);
    noiseReleaseCoeff = computeReleaseCoeff (1800.0f, sr);

    gainAttackCoeff  = computeAttackCoeff  (2.0f,   sr);
    gainReleaseCoeff = computeReleaseCoeff (120.0f, sr);

    bypassWet.reset (sr, 0.015);
    suppressionSmooth.reset (sr, 0.020);

    reset();
}

void NoiseGT1Processor::reset()
{
    signalEnvelope.fill (0.0f);
    noiseFloorEstimate.fill (1e-6f);
    currentGain = 1.0f;
    suppressionSmooth.setCurrentAndTargetValue (readParam (suppressionParam, suppressionAmount * 100.0f) / 100.0f);
    bypassWet.setCurrentAndTargetValue (readParam (bypassParam, bypassed ? 1.0f : 0.0f) > 0.5f ? 0.0f : 1.0f);
    gainReductionDb.store (0.0f);
}

void NoiseGT1Processor::setSuppressionAmount (float zeroToOne)
{
    suppressionAmount = juce::jlimit (0.0f, 1.0f, zeroToOne);
    suppressionSmooth.setTargetValue (suppressionAmount);
}

void NoiseGT1Processor::setBypass (bool shouldBypass)
{
    bypassed = shouldBypass;
    bypassWet.setTargetValue (shouldBypass ? 0.0f : 1.0f);
}

template <typename FloatType>
void NoiseGT1Processor::processInternal (juce::AudioBuffer<FloatType>& buffer)
{
    setSuppressionAmount (readParam (suppressionParam, suppressionAmount * 100.0f) / 100.0f);
    setBypass (readParam (bypassParam, bypassed ? 1.0f : 0.0f) > 0.5f);

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        suppressionAmount = suppressionSmooth.getNextValue();
        const float wetMix = bypassWet.getNextValue();
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

            const float signal = juce::jmax (signalEnvelope[ch], 1e-9f);
            const float noise  = juce::jmax (noiseFloorEstimate[ch], 1e-9f);
            const float signalToNoise = signal / noise;

            if (signalToNoise < 5.0f || signal < computeThreshold() * 2.0f)
            {
                if (absVal > noiseFloorEstimate[ch])
                    noiseFloorEstimate[ch] += noiseAttackCoeff  * (absVal - noiseFloorEstimate[ch]);
                else
                    noiseFloorEstimate[ch] += noiseReleaseCoeff * (absVal - noiseFloorEstimate[ch]);
            }
            else
            {
                noiseFloorEstimate[ch] += noiseReleaseCoeff * (0.25f * absVal - noiseFloorEstimate[ch]);
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
            const FloatType dry = channelData[sample];
            const FloatType wet = (FloatType) (dry * currentGain);
            channelData[sample] = (FloatType) (dry * (1.0f - wetMix) + wet * wetMix);
        }
    }

    const float grDb = (currentGain > 1e-6f) ? -20.0f * std::log10 (currentGain) : 72.0f;
    gainReductionDb.store (juce::jlimit (0.0f, 72.0f, grDb));
}

void NoiseGT1Processor::process (juce::AudioBuffer<float>& buffer)  { processInternal (buffer); }
void NoiseGT1Processor::process (juce::AudioBuffer<double>& buffer) { processInternal (buffer); }

float NoiseGT1Processor::computeThreshold() const noexcept
{
    const float shaped = std::pow (juce::jlimit (0.0f, 1.0f, suppressionAmount), 0.65f);
    const float threshDb = -78.0f + shaped * 60.0f;
    return juce::Decibels::decibelsToGain (threshDb);
}

float NoiseGT1Processor::computeGainTarget (float signalRms, float noiseRms) const noexcept
{
    const float amount = juce::jlimit (0.0f, 1.0f, suppressionAmount);
    if (amount < 0.001f)
        return 1.0f;

    const float threshold = computeThreshold();
    const float floorMultiplier = 2.0f + amount * 18.0f;
    const float effectiveThreshold = juce::jmax (threshold, noiseRms * floorMultiplier);

    if (signalRms >= effectiveThreshold)
        return 1.0f;

    const float ratio = juce::jlimit (0.0f, 1.0f, signalRms / juce::jmax (effectiveThreshold, 1e-9f));
    const float exponent = 1.15f + amount * 3.25f;
    float gainTarget = std::pow (ratio, exponent);
    const float maxReductionDb = 12.0f + amount * 48.0f;
    const float minGain = juce::Decibels::decibelsToGain (-maxReductionDb);
    return juce::jlimit (minGain, 1.0f, gainTarget);
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
