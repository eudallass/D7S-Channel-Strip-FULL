# Tier 4 — D7S Clipper FSC Rewrite Audit

Microtarefa 1/31.

## Objetivo

Reescrever o `ClipperProcessor` para seguir a referência do Fruity Soft Clipper:

- curva tanh simétrica;
- sem shape variável;
- sem bias drift;
- sem mix dry/wet interno;
- sem oversampling;
- latência zero;
- estrutura Pre → Threshold → Post.

## Arquivos alterados

- `Source/Modules/ClipperProcessor.h`
- `Source/Modules/ClipperProcessor.cpp`
- `Source/Core/ParameterLayout.cpp`
- `Source/PluginEditor.h`
- `Source/PluginEditor.cpp`

## Parâmetros antigos removidos

Busca no repo não encontrou mais:

- `clipper_drive`
- `clipper_ceiling`
- `clipper_shape`
- `clipper_mix`

## Parâmetros atuais

- `clipper_threshold` — -30.0 a 0.0 dB, default -4.4 dB
- `clipper_pre` — -12.0 a +24.0 dB, default 0.0 dB
- `clipper_post` — -24.0 a +24.0 dB, default 0.0 dB
- `clipper_bypass` — default false

## Algoritmo atual

```cpp
static float fsClip (float x, float thresholdLinear) noexcept
{
    constexpr float ceiling = 1.0f;
    const float t = juce::jlimit (0.001f, 0.9999f, thresholdLinear);
    const float range = ceiling - t;

    if (x > t)
        return t + range * std::tanh ((x - t) / range);

    if (x < -t)
        return -t + range * std::tanh ((x + t) / range);

    return x;
}
```

## Propriedades preservadas

- Simetria: `f(-x) = -f(x)`.
- Harmônicos pares minimizados.
- Ceiling assintótico em ±1.0.
- Threshold default em -4.4 dB.
- Latência reportada = 0.
- Bypass crossfade mantido.
- DC blocker mantido por segurança.

## Próximo teste necessário

- Null test contra Fruity Soft Clipper real.
- FFT com seno 1 kHz para confirmar pares abaixo de -80 dB.
