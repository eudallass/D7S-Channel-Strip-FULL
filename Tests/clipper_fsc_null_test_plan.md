# Clipper Null Test — Fruity Soft Clipper

Microtarefa 24/31.

Objetivo: verificar o D7S Clipper contra Fruity Soft Clipper real.

## Configuração D7S

- `clipper_threshold = -4.4 dB`
- `clipper_pre = 0 dB`
- `clipper_post = 0 dB`
- `clipper_bypass = false`
- Rack Mix = 100%
- Outros módulos bypassados

## Configuração FSC

- Threshold equivalente a -4.4 dB
- Sem ganho extra antes/depois

## Procedimento

1. Renderizar o mesmo áudio musical no FL Studio com Fruity Soft Clipper.
2. Renderizar o mesmo áudio no Ableton com D7S Clipper.
3. Alinhar samples.
4. Inverter polaridade de uma renderização.
5. Medir residual RMS/peak.

## Critério de aceitação

Residual esperado: menor que -40 dB em material musical.

## Observação

Este teste exige o Fruity Soft Clipper real no FL Studio e não pode ser executado dentro do GitHub Actions macOS sem FL Studio/plugin proprietário.
