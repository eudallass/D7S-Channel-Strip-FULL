# Analyzer Sub Frequency Visibility Test

Microtarefa 29/31.

Objetivo: garantir que o analyzer mostra sub frequencies dentro da área visível.

## Setup

Gerar tons:

- 20 Hz
- 30 Hz
- 40 Hz
- 50 Hz
- 60 Hz
- 80 Hz

## Procedimento

1. Inserir plugin no Ableton.
2. Ativar analyzer post.
3. Rodar tons um a um.
4. Confirmar que a energia aparece no canto esquerdo do analyzer sem cortar no topo ou sumir.

## Critério de aceitação

- 20 Hz aparece no limite esquerdo.
- 30/40/50/60 Hz aparecem claramente separados.
- Analyzer não corta a curva no topo.
- Range 60/90/120 dB deve alterar visibilidade sem esconder sub.
