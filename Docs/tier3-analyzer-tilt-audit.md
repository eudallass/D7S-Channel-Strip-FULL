# Tier 3 Analyzer Tilt Audit

Microtarefa 8/32.

Arquivo auditado: `Source/UI/SpectrumDisplay.cpp`

Resultado:

- O processor entrega os bins já suavizados em dB.
- O tilt é aplicado no método `SpectrumDisplay::displayDbForBin()`.
- A aplicação ocorre antes do mapeamento Y em pixels.
- O pivô está correto em 1 kHz:

```cpp
const float oct = std::log2 (juce::jmax (freqHz, 20.0f) / 1000.0f);
return (dbFs - refDb) + tilt * oct;
```

Status: fórmula e ordem de aplicação corretas.
