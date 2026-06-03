#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

/**
 * IModule -- Common interface for all D7S Channel Strip rack modules.
 *
 * Every DSP module (NoiseGT1, EQ4K, Comp76, LA2A, Tube, Esser, Delay, ...)
 * implements this interface so the ModuleChain can process them
 * polymorphically and reorder them via drag-and-drop without the
 * PluginProcessor needing to know each module type explicitly.
 *
 * Contract:
 *  - prepare()           : called once before playback (or on sample-rate
 *                          / block-size change). Reset smoothers, allocate
 *                          buffers, configure filters here. Audio-thread-safe
 *                          after this returns.
 *  - reset()             : called by the host on transport stop or by the
 *                          chain when needed. Must reset all internal state
 *                          but keep configuration (sample rate, block size).
 *  - process<FloatType>  : called from the audio thread, every audio block.
 *                          Implementations MUST be lock-free and allocation-free.
 *                          The buffer is processed in-place.
 *  - setBypass(bool)     : flips an internal SmoothedValue that performs a
 *                          short crossfade between dry and wet inside process(),
 *                          so toggling bypass produces ZERO clicks.
 *  - getLatencySamples() : reports the latency this module introduces
 *                          (lookahead + oversampling). Default 0.
 *  - getIdentifier()     : stable string id of the module
 *                          (e.g. "noisegt1", "eq4k", "comp76", "la2a",
 *                          "tube", "esser", "delay"). Used by the chain
 *                          for state save/load and for module-order serialisation.
 *
 * Threading rules:
 *  - prepare()/reset() : message thread (audio thread is stopped).
 *  - process()         : audio thread only. NEVER allocate, lock, or call JUCE
 *                        objects that allocate (e.g. juce::String concat).
 *  - setBypass()/setters of parameters : message thread (called by
 *                        PluginProcessor::processBlock 1x per buffer after
 *                        reading cached APVTS pointers). Implementations must
 *                        accept them from any thread (use atomics or
 *                        SmoothedValue setTargetValue, which is lock-free).
 */
class IModule
{
public:
    virtual ~IModule() = default;

    //==========================================================================
    // Lifecycle
    virtual void prepare (double sampleRate, int blockSize, int numChannels) = 0;
    virtual void reset()                                                     = 0;

    //==========================================================================
    // Audio processing (in-place). Both precisions supported.
    virtual void process (juce::AudioBuffer<float>&  buffer) = 0;
    virtual void process (juce::AudioBuffer<double>& buffer) = 0;

    //==========================================================================
    // Module bypass with internal crossfade (15 ms by default in implementations).
    virtual void setBypass (bool shouldBypass) = 0;

    //==========================================================================
    // Latency this module introduces in samples (lookahead + oversampling).
    // Default 0 -- override in modules that buffer the signal.
    virtual int getLatencySamples() const noexcept { return 0; }

    //==========================================================================
    // Stable id used by ModuleChain for ordering and serialisation.
    // Lowercase, no spaces, no special chars.
    virtual const juce::String getIdentifier() const = 0;
};
