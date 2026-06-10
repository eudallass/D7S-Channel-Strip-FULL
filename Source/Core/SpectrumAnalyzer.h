#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>

/**
 *  High-quality spectrum analyzer in the style of FabFilter Pro-Q 4 / Waves PAZ Analyzer.
 *
 *  - 8192-point FFT with 75% overlap (hop = 2048) for smooth, high-resolution display.
 *  - Hann window with coherent-gain correction so a 0 dBFS sine reads 0 dB on the meter.
 *  - 1/24-octave smoothing in log-frequency domain (peak within band for detail).
 *  - +4.5 dB/octave tilt (Pro-Q 4 default) referenced at 1 kHz.
 *  - Configurable decay (default 30 dB/s) — instant attack, smooth fall.
 *  - Lock-free audio -> UI thread handoff via std::atomic indices.
 *  - Zero added latency to the audio path: the analyzer only observes samples.
 *  - FFT runs on the UI timer thread, NEVER on the audio thread.
 */
class SpectrumAnalyzer
{
public:
    static constexpr int fftOrder  = 13;
    static constexpr int fftSize   = 1 << fftOrder;
    static constexpr int hopSize   = fftSize / 4;
    static constexpr int scopeSize = 512;

    SpectrumAnalyzer();

    void prepare (double newSampleRate);
    void reset();

    void pushSample (float sample) noexcept;
    void pushBuffer (const float* leftChannel,
                     const float* rightChannel,
                     int numSamples) noexcept;

    bool processFFTIfReady();

    const std::array<float, scopeSize>& getDisplayData() const noexcept { return displayData; }

    void setSmoothingOctaveFraction (float fraction) noexcept { smoothingOct = fraction; }
    void setDecayDbPerSecond       (float dbPerSec)  noexcept { decayDbPerSec = dbPerSec; }
    void setTiltDbPerOctave        (float dbPerOct)  noexcept { tiltDbPerOctave = dbPerOct; }
    void setTiltReferenceHz        (float hz)        noexcept { tiltReferenceHz = hz; }
    void setFrequencyRange         (float lowHz, float highHz) noexcept { minFreq = lowHz; maxFreq = highHz; }
    void setDecibelRange           (float lowDb, float highDb) noexcept { minDb = lowDb; maxDb = highDb; }

    float getMinFreq() const noexcept { return minFreq; }
    float getMaxFreq() const noexcept { return maxFreq; }
    float getMinDb()   const noexcept { return minDb; }
    float getMaxDb()   const noexcept { return maxDb; }

private:
    void computeFFT();
    void mapToLogScopeAndDecay();

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize,
                                                  juce::dsp::WindowingFunction<float>::hann };

    std::array<float, fftSize> fifoBuffer {};
    std::atomic<int>  fifoWriteIndex      { 0 };
    std::atomic<int>  samplesSinceLastHop { 0 };
    std::atomic<bool> nextFFTReady        { false };

    std::array<float, 2 * fftSize> fftWorkBuffer {};
    std::array<float, fftSize / 2> magnitudeSpectrum {};
    std::array<float, scopeSize> displayData {};

    double sampleRate       = 44100.0;
    float  minFreq          = 20.0f;
    float  maxFreq          = 20000.0f;
    float  minDb            = -100.0f;
    float  maxDb            = 6.0f;
    float  smoothingOct     = 1.0f / 24.0f;
    float  decayDbPerSec    = 30.0f;
    float  tiltDbPerOctave  = 4.5f;
    float  tiltReferenceHz  = 1000.0f;

    juce::int64 lastFrameTimeMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzer)
};
