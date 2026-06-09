# Tier 4 — D7S Clipper FSC Rewrite

Microtarefa 1/31.

O `ClipperProcessor` foi reescrito para seguir a estrutura sonora do Fruity Soft Clipper:

- algoritmo tanh simétrico;
- threshold default em -4.4 dB;
- estrutura Pre → Threshold → Post;
- sem shape variável;
- sem mix interno;
- sem bias drift/modulação assimétrica;
- sem oversampling/lookahead;
- latência reportada 0 samples;
- bypass com crossfade preservado;
- DC blocker mantido por segurança.

Parâmetros atuais:

- `clipper_threshold` — -30 a 0 dB, default -4.4 dB
- `clipper_pre` — -12 a +24 dB, default 0 dB
- `clipper_post` — -24 a +24 dB, default 0 dB
- `clipper_bypass` — default false

Parâmetros antigos removidos:

- `clipper_drive`
- `clipper_ceiling`
- `clipper_shape`
- `clipper_mix`

Busca no repo confirmou que não restaram referências aos parâmetros antigos.
