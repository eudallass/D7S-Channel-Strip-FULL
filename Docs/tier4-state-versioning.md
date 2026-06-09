# Tier 4 — State Versioning / Migration Plan

Microtarefa 13/31.

Estado alvo:

- `d7s_state_version = 4`
- `plugin_version = JucePlugin_VersionString`

Migrações previstas:

## Clipper Tier 3 → Tier 4

Parâmetros antigos do Clipper híbrido devem ser descartados:

- `clipper_drive`
- `clipper_ceiling`
- `clipper_shape`
- `clipper_mix`

Parâmetros novos FSC:

- `clipper_threshold` default -4.4 dB
- `clipper_pre` default 0 dB
- `clipper_post` default 0 dB
- `clipper_bypass` default false

## Delay legacy → Delay modo novo

Parâmetro legacy:

- `delay_time`

Parâmetros novos:

- `delay_mode`
- `delay_fraction_index`
- `delay_time_ms`

Mapeamento recomendado:

- legacy 1/32 → mode Note, fraction 1/32
- legacy 1/16 → mode Note, fraction 1/16
- legacy 1/8 → mode Note, fraction 1/8
- legacy 1/4 → mode Note, fraction 1/4
- legacy 1/2 → mode Note, fraction 1/2
- legacy 1/1 → mode Note, fraction 1/1
- legacy triplet → mode Triplet
- legacy dotted → mode Dotted

Observação: a implementação direta no `PluginProcessor_Tier1.cpp` deve ser feita em patch isolado após build verde do Tier 4 atual, porque o arquivo está grande e concentra analyzer thread + state + processBlock.
