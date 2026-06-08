# Tier 3 Analyzer Contrast Audit

Microtarefa 10/32.

O contraste foi ajustado junto da correção de Y mapping no commit anterior:

- Stroke das curvas aumentado para 1.8px.
- Fill alpha aumentado para ~60/255 (`0.235f`).
- Linha 0 dB de referência agora é âmbar sólido com 1.5px.
- A curva POST continua desenhada depois da PRE, portanto fica visualmente por cima.

Status: requisitos de contraste aplicados em `Source/UI/SpectrumDisplay.cpp`.
