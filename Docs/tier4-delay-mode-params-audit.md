# Tier 4 — Delay Mode Params Audit

Microtarefa 6/31.

Status confirmado no código:

- `delay_mode` existe no `ParameterLayout.cpp` como Choice: Msec / Note / Dotted / Triplet.
- `delay_fraction_index` existe no `ParameterLayout.cpp` como índice discreto 0..6.
- `delay_time_ms` existe no `ParameterLayout.cpp` como tempo em milissegundos.
- `delay_time` legacy continua presente como `Delay Time Legacy` para compatibilidade de state.
- `DelayGlideProcessor` declara e implementa:
  - `setDelayMode(int)`
  - `setDelayFractionIndex(int)`
  - `setDelayTimeMs(float)`
  - `getModeBeats()`
  - `getUserDelaySeconds()`
- `PluginProcessor_Tier1.cpp` já alimenta o delay com os novos parâmetros:
  - `delayGlide.setDelayMode(...)`
  - `delayGlide.setDelayFractionIndex(...)`
  - `delayGlide.setDelayTimeMs(...)`

Resultado: os novos parâmetros de modo/fração/tempo estão prontos para a UI redesenhada da microtarefa 7/31.
