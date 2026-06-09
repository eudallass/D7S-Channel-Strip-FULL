# Delay Tape Pitch Transition Test

Microtarefa 28/31.

Objetivo: validar o comportamento tape/glide do D7S Delay.

## Setup

- Loop musical tocando.
- Delay Mix: 25%.
- Feedback: 35%.
- Mode: Note.
- Alternar fração entre 1/16, 1/8, 1/4 e 1/2.
- Glide ligado.

## Critério de aceitação

Ao trocar a fração com áudio tocando:

- deve haver transição de pitch audível;
- a transição deve ser musical, não click ou zipper noise;
- o tempo deve estabilizar em aproximadamente 100 ms;
- wow/flutter deve dar movimento sutil sem desafinar demais.

## Observação

O pitch transitório vem da combinação:

- `juce::dsp::DelayLine<Lagrange3rd>`;
- delay time suavizado sample-by-sample;
- wow/flutter compartilhado.
