# Tier 3 — Build/Test Checkpoint

Último commit preparado para build: `df308ffd8c5eb9b818c787efc0b1d03a9a016a3f`

Branch: `dev/all-module-params`

## Objetivo deste checkpoint

Validar a versão atual antes de seguir para as últimas etapas do Tier 3.

## Principais mudanças desde o último teste

- Rack com 8 módulos.
- Novo módulo `D7S Clipper` integrado ao DSP/runtime/UI.
- Controles do Clipper:
  - Drive
  - Ceiling
  - Shape
  - Mix
  - Bypass
- Scroll horizontal do rack via `juce::Viewport`.
- Indicadores de scroll nas bordas.
- Scrollbar customizada D7S.
- Auto-scroll durante drag-and-drop.
- Ordem dos módulos salva/restaurada no state da DAW via `d7s_module_order`.
- UI abre sincronizada com a ordem restaurada pelo processor.
- Sanitização de ordem inválida/duplicada.
- Correções preventivas de build para JUCE 7.0.9:
  - removido `setScrollOnDragMode(...)`
  - trocado `juce::FontOptions` por `juce::Font(...)`
- `editor_width` e `editor_height` marcados como meta/non-automatable.

## Checklist de teste no Ableton

1. Plugin abre sem crash.
2. Todos os 8 módulos aparecem no rack horizontal.
3. Scroll horizontal funciona.
4. Shift + wheel move horizontalmente, se suportado pelo host/sistema.
5. Drag-and-drop ainda reordena módulos.
6. Reordenar módulos altera o resultado sonoro conforme ordem esquerda → direita.
7. Salvar projeto, fechar e reabrir: ordem dos módulos deve permanecer igual.
8. Clipper aparece com Drive/Ceiling/Shape/Mix/Bypass.
9. Clipper processa áudio:
   - Drive aumenta densidade/saturação.
   - Ceiling limita o teto.
   - Shape muda caráter entre mais hard e mais soft/round.
   - Mix mistura dry/wet.
10. Parâmetros `editor_width` e `editor_height` não devem aparecer como automação musical útil.
11. Bypass individual dos módulos continua funcionando.
12. Bypass do rack/host continua sem clicks evidentes.

## Observações

Se o build falhar, primeiro investigar:

- compatibilidade de `AudioParameterFloatAttributes` no JUCE 7.0.9;
- erros em `PluginEditor.cpp`, porque ele está grande e concentrado;
- erros de constructor/overload em parâmetros;
- erro de assinatura/codesign apenas depois que o build compilar, lembrando o aprendizado anterior do `moduleinfo.json`.

## Próximo passo após build verde

Seguir para 30/32: testar/ajustar compatibilidade do parâmetro meta caso o GitHub Actions acuse erro.
