# Tier 3 — Final Audit

Branch: `dev/all-module-params`

Último commit funcional preparado: `ad7f41a27cb82dd71ccf1ddb866c2645373af6ec`

## Status geral

Tier 3 avançou a arquitetura do D7S Channel Strip FULL para um rack mais próximo da filosofia Slate VMR:

- rack horizontal;
- módulos reordenáveis por drag-and-drop;
- signal flow seguindo a ordem visual esquerda → direita;
- persistência de ordem no state da DAW;
- scroll horizontal;
- novo módulo D7S Clipper;
- melhorias de build safety para JUCE 7.0.9.

## Módulos atuais no rack

1. D7S NoiseGT1
2. D7S EQ 4K
3. D7S 76
4. D7S 2A
5. D7S Tube
6. D7S Clipper
7. D7S Esser
8. D7S Delay

## Principais entregas

### Rack / UI

- `RackViewport` com scrollbar horizontal.
- Shift + wheel tentando converter rolagem vertical em horizontal.
- Indicadores visuais nas bordas quando existe conteúdo fora da área visível.
- Scrollbar customizada D7S.
- Auto-scroll enquanto arrasta módulos.
- Visual polish nos headers dos módulos.
- UI lê a ordem atual do processor ao abrir.

### Drag-and-drop / signal flow

- `editorModuleOrder` controla a ordem visual.
- `audioProcessor.setModuleOrder()` atualiza a ordem DSP.
- `processAudioBlock()` processa os módulos na ordem do array restaurado.
- Ordem visual e ordem DSP ficam sincronizadas.

### Persistência

- `getStateInformation()` salva `d7s_module_order`.
- `setStateInformation()` restaura `d7s_module_order`.
- `setModuleOrder()` sanitiza duplicados, IDs inválidos e módulos faltantes.

### D7S Clipper

Arquivos adicionados:

- `Source/Modules/ClipperProcessor.h`
- `Source/Modules/ClipperProcessor.cpp`

Parâmetros:

- `clipper_drive`
- `clipper_ceiling`
- `clipper_shape`
- `clipper_mix`
- `clipper_bypass`

DSP atual:

- hard/soft hybrid clip;
- ceiling em dBFS;
- drive em dB;
- shape para caráter mais hard ou mais rounded;
- mix dry/wet;
- bypass crossfade via `bypassWet`;
- DC blocker simples;
- leve bias drift.

### Build safety

Correções preventivas aplicadas:

- Removido `setScrollOnDragMode(...)`.
- Substituído `juce::FontOptions` por `juce::Font(...)`.
- Removidas declarações antigas de latência sem implementação.
- Revertido uso de `AudioParameterFloatAttributes` nos parâmetros de tamanho de editor, porque a busca no repo não encontrou essa API.

## Atenção

`editor_width` e `editor_height` continuam como parâmetros normais por compatibilidade de build. Mais tarde podemos mover persistência de tamanho para o XML/state customizado em vez de APVTS, eliminando esses parâmetros da DAW.

## Checklist de teste obrigatório

1. Build macos-14 arm64 fica verde.
2. Plugin aparece no Ableton.
3. Plugin abre sem crash.
4. Todos os 8 módulos aparecem.
5. Scroll horizontal funciona.
6. Drag-and-drop funciona.
7. Ordem visual muda o som.
8. Salvar/reabrir projeto preserva ordem dos módulos.
9. D7S Clipper aparece e processa áudio.
10. Bypass do Clipper funciona.
11. Bypass dos outros módulos continua funcionando.
12. Delay ainda funciona depois da adição do Clipper.
13. Meters/analyzer continuam vivos.
14. Nenhum parâmetro crítico desapareceu no Ableton.

## Próximos ajustes após teste

- Se build falhar: ler logs e corrigir compile error específico.
- Se UI abrir grande demais: ajustar `designWidth` e escalas.
- Se Clipper soar forte/fraco demais: calibrar drive, trim e curve.
- Se editor_width/editor_height incomodarem na automação: migrar tamanho de janela para XML/state customizado.
- Se CPU subir: otimizar analyzer e Clipper.
