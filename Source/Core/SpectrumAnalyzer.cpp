#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer()
{
    displayData.fill (minDb);
    for (auto& state : snapshotStates)
        state.store (snapshotFree, std::memory_order_relaxed);

    lastFrameTimeMs = juce::Time::currentTimeMillis();
}

void SpectrumAnalyzer::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void SpectrumAnalyzer::reset()
{
    fifoBuffer.fill (0.0f);
    for (auto& snapshot : frameSnapshots)
        snapshot.fill (0.0f);

    fftWorkBuffer.fill (0.0f);
    magnitudeSpectrum.fill (0.0f);
    displayData.fill (minDb);

    fifoWriteIndex.store (0, std::memory_order_relaxed);
    samplesSinceLastHop.store (0, std::memory_order_relaxed);
    latestReadySnapshot.store (-1, std::memory_order_release);
    nextSnapshotSearchSlot = 0;

    for (auto& state : snapshotStates)
        state.store (snapshotFree, std::memory_order_release);

    lastFrameTimeMs = juce::Time::currentTimeMillis();
}

void SpectrumAnalyzer::pushSample (float sample) noexcept
{
    const int idx = fifoWriteIndex.load (std::memory_order_relaxed);
    fifoBuffer[(size_t) idx] = sample;
    fifoWriteIndex.store ((idx + 1) % fftSize, std::memory_order_relaxed);

    const int hopCount = samplesSinceLastHop.fetch_add (1, std::memory_order_relaxed) + 1;
    if (hopCount >= hopSize)
    {
        samplesSinceLastHop.store (0, std::memory_order_relaxed);
        buildSnapshotFromFifo();
    }
}

void SpectrumAnalyzer::pushBuffer (const float* L, const float* R, int n) noexcept
{
    if (L == nullptr || n <= 0)
        return;

    if (R == nullptr)
    {
        for (int i = 0; i < n; ++i)
            pushSample (L[i]);
    }
    else
    {
        for (int i = 0; i < n; ++i)
            pushSample (0.5f * (L[i] + R[i]));
    }
}

int SpectrumAnalyzer::acquireFreeSnapshotSlot() noexcept
{
    for (int attempt = 0; attempt < numSnapshots; ++attempt)
    {
        const int slot = (nextSnapshotSearchSlot + attempt) % numSnapshots;
        int expected = snapshotFree;

        if (snapshotStates[(size_t) slot].compare_exchange_strong (expected,
                                                                    snapshotWriting,
                                                                    std::memory_order_acq_rel,
                                                                    std::memory_order_relaxed))
        {
            nextSnapshotSearchSlot = (slot + 1) % numSnapshots;
            return slot;
        }
    }

    return -1;
}

void SpectrumAnalyzer::buildSnapshotFromFifo() noexcept
{
    const int slot = acquireFreeSnapshotSlot();
    if (slot < 0)
        return;

    const int writeIdx = fifoWriteIndex.load (std::memory_order_acquire);
    auto& snapshot = frameSnapshots[(size_t) slot];

    for (int i = 0; i < fftSize; ++i)
    {
        const int readIdx = (writeIdx + i) % fftSize;
        snapshot[(size_t) i] = fifoBuffer[(size_t) readIdx];
    }

    publishSnapshot (slot);
}

void SpectrumAnalyzer::publishSnapshot (int slot) noexcept
{
    snapshotStates[(size_t) slot].store (snapshotReady, std::memory_order_release);

    const int previousReady = latestReadySnapshot.exchange (slot, std::memory_order_acq_rel);
    if (previousReady >= 0 && previousReady != slot)
        snapshotStates[(size_t) previousReady].store (snapshotFree, std::memory_order_release);
}

bool SpectrumAnalyzer::processFFTIfReady()
{
    const int slot = latestReadySnapshot.exchange (-1, std::memory_order_acq_rel);
    if (slot < 0)
        return false;

    snapshotStates[(size_t) slot].store (snapshotReading, std::memory_order_release);

    const auto& snapshot = frameSnapshots[(size_t) slot];
    for (int i = 0; i < fftSize; ++i)
        fftWorkBuffer[(size_t) i] = snapshot[(size_t) i];

    snapshotStates[(size_t) slot].store (snapshotFree, std::memory_order_release);

    computeFFT();
    mapToLogScopeAndDecay();
    return true;
}

void SpectrumAnalyzer::computeFFT()
{
    std::fill (fftWorkBuffer.begin() + fftSize, fftWorkBuffer.end(), 0.0f);
    window.multiplyWithWindowingTable (fftWorkBuffer.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftWorkBuffer.data());

    // A unit sine in a real N-point FFT gives N/2. Hann coherent gain is 0.5,
    // so the coherent-amplitude-correct scale is 4/N.
    const float scale = 4.0f / (float) fftSize;
    for (int bin = 0; bin < fftSize / 2; ++bin)
        magnitudeSpectrum[(size_t) bin] = fftWorkBuffer[(size_t) bin] * scale;
}

void SpectrumAnalyzer::mapToLogScopeAndDecay()
{
    const juce::int64 now      = juce::Time::currentTimeMillis();
    const float       deltaSec = juce::jlimit (0.001f, 0.1f, (float) (now - lastFrameTimeMs) * 0.001f);
    lastFrameTimeMs            = now;
    const float decayThisFrame = decayDbPerSec * deltaSec;

    const float logMinFreq        = std::log (minFreq);
    const float logMaxFreq        = std::log (maxFreq);
    const float halfBandFactor    = std::pow (2.0f, smoothingOct * 0.5f);
    const float invHalfBandFactor = 1.0f / halfBandFactor;
    const float binsPerHz         = (float) fftSize / (float) sampleRate;
    const int   maxBin            = fftSize / 2 - 1;
    const float invLog2           = 1.0f / std::log (2.0f);

    for (int x = 0; x < scopeSize; ++x)
    {
        const float t    = (float) x / (float) (scopeSize - 1);
        const float freq = std::exp (logMinFreq + t * (logMaxFreq - logMinFreq));

        const float freqLow  = freq * invHalfBandFactor;
        const float freqHigh = freq * halfBandFactor;
        const int   binLow   = juce::jlimit (1, maxBin, (int) std::floor (freqLow  * binsPerHz));
        const int   binHigh  = juce::jlimit (1, maxBin, (int) std::ceil  (freqHigh * binsPerHz));

        float peakMag = 0.0f;
        for (int b = binLow; b <= binHigh; ++b)
            peakMag = std::max (peakMag, magnitudeSpectrum[(size_t) b]);

        float newDb = juce::Decibels::gainToDecibels (peakMag, minDb);

        const float tiltOctaves = std::log (freq / tiltReferenceHz) * invLog2;
        newDb += tiltDbPerOctave * tiltOctaves;

        const float decayed = displayData[(size_t) x] - decayThisFrame;
        displayData[(size_t) x] = std::max (newDb, decayed);
    }
}
