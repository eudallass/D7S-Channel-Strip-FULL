# pluginval Strictness 10 — Multi Sample Rate Test

Microtarefa 30/31.

Objetivo: validar pluginval strictness 10 em múltiplos sample rates.

## Sample rates alvo

- 44.1 kHz
- 48 kHz
- 96 kHz
- 192 kHz

## Critério de aceitação

- `pluginval --strictness-level 10 --validate-in-process` passa no CI.
- Sem crash em sample rate switching.
- Sem denormal/stall em silêncio.
- Sem erro de state save/load.
- Sem erro de editor open/close.

## Workflow atual

`.github/workflows/macos-build.yml` já foi atualizado para strictness 10.

## Observação

A validação por sample rate específico depende do comportamento interno do pluginval e/ou de teste local complementar em host. O CI já falha se pluginval strictness 10 encontrar problema crítico.
