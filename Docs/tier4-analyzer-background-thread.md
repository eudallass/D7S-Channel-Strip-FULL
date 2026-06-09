# Tier 4 — Analyzer Background Thread

Microtarefa 8/31.

Antes:

- `pushPreSpectrumSample()` e `pushPostSpectrumSample()` acumulavam amostras e chamavam `runPreSpectrumFFT()` / `runPostSpectrumFFT()` diretamente quando o buffer enchia.
- Isso fazia a FFT rodar no audio thread.

Agora:

- `D7SChannelStripFullAudioProcessor` herda de `juce::Thread`.
- A thread se chama `D7S Analyzer Thread`.
- O audio thread apenas empurra samples em `juce::AbstractFifo`:
  - `preAnalyzerFifo`
  - `postAnalyzerFifo`
- A thread de analyzer consome os FIFOs, monta blocos de `spectrumFFTSize` e chama:
  - `runPreSpectrumFFT()`
  - `runPostSpectrumFFT()`
- A UI continua lendo atomics do `AnalyzerState`.

Resultado esperado:

- Menos carga no audio thread.
- FFT fora do caminho crítico de áudio.
- Melhor estabilidade em buffers pequenos.
