# Tier 4 — OpenGL Context Note

Microtarefa 9/31.

Foi adicionado o link do módulo `juce::juce_opengl` no CMake para preparar o editor para `juce::OpenGLContext`.

Por segurança, a etapa de attach/detach no `PluginEditor.cpp` deve ser aplicada depois de validar o build atual, porque o arquivo do editor está grande e concentrado. O próximo patch recomendado é:

- incluir `<juce_opengl/juce_opengl.h>` no `PluginEditor.h`;
- adicionar `juce::OpenGLContext openGLContext;` como membro;
- no construtor:
  - `openGLContext.setContinuousRepainting (true);`
  - `openGLContext.attachTo (*this);`
- no destrutor:
  - `openGLContext.detach();`

Objetivo: animações mais fluidas no analyzer e meters.
