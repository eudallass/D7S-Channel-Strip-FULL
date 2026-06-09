# Clipper FFT Harmonics Test

Microtarefa 25/31.

Objetivo: validar simetria do D7S Clipper estilo Fruity Soft Clipper.

## Setup

- Seno 1 kHz a -2 dBFS.
- `clipper_threshold = -4.4 dB`
- `clipper_pre = 0 dB`
- `clipper_post = 0 dB`
- Outros módulos bypassados.

## Expectativa

Por ser tanh simétrico:

- Harmônicos ímpares devem aparecer:
  - 3 kHz
  - 5 kHz
  - 7 kHz
- Harmônicos pares devem ser praticamente inexistentes:
  - 2 kHz
  - 4 kHz
  - 6 kHz

## Critério de aceitação

- Harmônicos pares <= -80 dB.
- Harmônicos ímpares claramente visíveis.

## Justificativa

A função `fsClip(x)` implementada é ímpar:

`f(-x) = -f(x)`

Isso gera predominância de harmônicos ímpares e cancela pares em condições ideais.
