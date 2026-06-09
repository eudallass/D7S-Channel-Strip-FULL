# Tier 4 — CPU Optimization Pass

Microtarefa 12/31.

Objetivo: manter CPU abaixo de 6% em 48 kHz / buffer 256 com todos os módulos ativos e analyzer em Medium.

## Otimizações já aplicadas neste Tier

- FFT do analyzer movida para `D7S Analyzer Thread`, fora do audio thread.
- Audio thread agora só faz push em `juce::AbstractFifo`.
- Delay usa `juce::dsp::DelayLine<Lagrange3rd>`, evitando interpolação manual custom.
- Delay recebeu anti-denormal noise no feedback network.
- Rack meters usam decay exponencial simples, sem trabalho pesado no UI timer.
- Clipper FSC reescrito sem oversampling, lookahead ou modulação, mantendo perfil CPU-friendly.

## Hot paths revisados

- `processAudioBlock()`
- `DelayGlideProcessor::process()`
- `ClipperProcessor::processInternal()`
- `TubeProcessor::processInternal()`
- analyzer FFT path

## Pontos ainda recomendados para profiler real no Mac

Rodar Instruments.app com:

- Sample rate: 48 kHz
- Buffer: 256
- Todos módulos ativos
- Analyzer: Medium
- Material: white noise ou loop full-band

Procurar funções >0.5% CPU:

- `std::tanh` no Clipper/Tube
- `juce::Decibels::decibelsToGain` em hot path
- delay FDN com 8 linhas x canais
- analyzer UI paint

## Critério de aceitação

Se Instruments mostrar CPU acima da meta, próximos candidatos de otimização:

1. cachear conversões dB → gain por bloco;
2. trocar `std::tanh` por aproximação racional opcional;
3. reduzir linhas FDN em modo low CPU;
4. reduzir FPS do analyzer em hosts pesados.
