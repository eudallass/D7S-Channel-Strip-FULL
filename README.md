# D7S Channel Strip FULL

Plugin **VST3 macOS arm64** em JUCE 7.0.9, inspirado no formato de rack modular/channel strip.

Escopo atual de build: **VST3 only / macOS arm64 only**.

## Módulos atuais

1. D7S NoiseGT1 — gate/suppression
2. D7S EQ 4K — EQ estilo console 4K
3. D7S 76 — compressor FET
4. D7S 2A — compressor óptico
5. D7S Tube — saturação tube com bias drift
6. D7S Clipper — soft clipper tanh simétrico estilo Fruity Soft Clipper
7. D7S Esser — de-esser wide/split
8. D7S Delay — delay tape/glide com modo Msec/Note/Dotted/Triplet

## Instalação macOS

Baixe o artifact `D7S-VST3-Mac-ARM64.zip` do GitHub Actions e rode:

```bash
chmod +x Scripts/install_d7s.sh
./Scripts/install_d7s.sh ~/Downloads/D7S-VST3-Mac-ARM64.zip
```

O script:

- fecha Ableton Live, se estiver aberto;
- copia o `.vst3` para `/Library/Audio/Plug-Ins/VST3`;
- remove quarentena;
- limpa entrada antiga do banco de plugins do Ableton quando possível.

## Presets de fábrica

- Vocal Pop Clean
- Vocal Rap Aggressive
- Drum Bus Glue
- Master Glue Sutil
- Acoustic Guitar Natural
- Bass DI Punchy

## Sample rates e buffers suportados

Alvo de validação:

- 44.1 kHz
- 48 kHz
- 96 kHz
- 192 kHz
- buffer 64, 128, 256, 512 samples

Meta de performance: CPU abaixo de 6% em 48 kHz / buffer 256 com todos os módulos ativos e analyzer em Medium.

## Latência por módulo

| Módulo | Latência esperada |
|---|---:|
| NoiseGT1 | interna conforme módulo |
| EQ 4K | 0 samples |
| D7S 76 | 0 samples |
| D7S 2A | 0 samples |
| D7S Tube | 0 samples |
| D7S Clipper | 0 samples |
| D7S Esser | 0 samples |
| D7S Delay | 0 samples reportados / tail musical |

## Referências sonoras

- Waves NS1 — referência conceitual para NoiseGT1
- SSL 4000 — referência conceitual para EQ 4K
- UAD/Waves 1176 / CLA-76 — referência conceitual para D7S 76
- UAD/Waves LA-2A / CLA-2A — referência conceitual para D7S 2A
- Fruity Soft Clipper — referência absoluta para D7S Clipper
- Valhalla Supermassive — referência de UX/modo de delay
- FabFilter Pro-Q — referência visual de analyzer

## Build

```bash
cmake -B build -S . -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

GitHub Actions aplica ad-hoc codesign antes do upload do artifact.
