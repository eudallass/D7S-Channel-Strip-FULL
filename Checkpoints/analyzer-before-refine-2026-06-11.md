# Checkpoint — Analyzer before refinement

Date: 2026-06-11
Branch: dev/all-module-params
Scope: checkpoint before starting analyzer refinement patches.

## Current analyzer baseline

- Core analyzer files already exist:
  - Source/Core/SpectrumAnalyzer.h
  - Source/Core/SpectrumAnalyzer.cpp
  - Source/UI/SpectrumAnalyzerComponent.h
  - Source/UI/SpectrumAnalyzerComponent.cpp
- Current design target:
  - 8192-point FFT
  - 75% overlap / hop 2048
  - Hann window coherent-gain correction
  - 1/24-octave smoothing
  - +4.5 dB/oct tilt referenced at 1 kHz
  - 30 dB/s decay
  - Pre/Post analyzer
  - zero added audio latency
  - FFT outside the audio thread

## Reason for checkpoint

The current analyzer is technically functional, but needs refinement to behave and look more like professional references such as FabFilter Pro-Q, Waves PAZ, iZotope Insight and SSL-style metering.

Next patches will focus on:

1. Safer audio-to-UI buffering / snapshot architecture.
2. More musical smoothing and response behavior.
3. More professional log-frequency grid and visual hierarchy.
4. User-facing analyzer controls: speed, smoothing, tilt, range, hold and Pre/Post view.
