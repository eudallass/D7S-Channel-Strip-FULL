# Tier 4 — Preset Header UI

Microtarefa 16/31.

Design alvo para o header:

`[ < ] [ Preset Name ▾ ] [ > ] [ Save ] [ Save As ]`

Comportamento:

- `<` carrega preset anterior.
- `>` carrega próximo preset.
- ComboBox lista Factory primeiro e User depois.
- Nome recebe `*` quando `PresetManager::isPresetDirty()` retorna true.
- Save sobrescreve preset user.
- Factory presets usam Save As.

Estado atual:

- `PresetManager` existe.
- Factory preset directory existe.
- A integração visual no `PluginEditor` deve ser aplicada depois do próximo build verde porque o editor foi reescrito na etapa 7/31 e ainda precisa validação visual no host.
