# Tier 4 — CI Codesign

Microtarefas 18/31 e 19/31.

O workflow `.github/workflows/macos-build.yml` agora:

- roda em `macos-14`;
- configura `CMAKE_OSX_ARCHITECTURES=arm64`;
- mantém build VST3 only;
- aplica ad-hoc codesign no bundle VST3 antes do upload;
- imprime `✅ Codesign applied`;
- roda `codesign -dv --verbose=4` para aparecer no log;
- roda `codesign --verify --deep --strict --verbose=4`.

Isto reduz a necessidade de codesign manual após baixar o artifact.
