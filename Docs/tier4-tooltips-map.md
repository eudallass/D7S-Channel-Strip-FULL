# Tier 4 — Tooltip Map

Microtarefa 17/31.

Tooltips recomendados por módulo:

## Clipper

- PRE: input gain before Fruity Soft Clipper curve. Default 0 dB.
- THRESHOLD: soft clipping threshold. Default -4.4 dB, FSC target.
- POST: output trim after clipping. Default 0 dB.

## Tube

- Beauty: upper harmonic tube sweetness.
- Beast: low-mid harmonic density.
- Sensitivity: input drive into tube stage.
- Mix: dry/wet blend.

## Delay

- Mode: Msec / Note / Dotted / Triplet.
- TIME: fraction selector or milliseconds depending on mode.
- Mix: delay wet level.
- Feedback: regeneration amount.
- Glide: enables pitch transition movement.

## Dynamics

- 76 Input/Output/Attack/Release/Ratio: FET compression controls.
- 2A Peak/Gain/Mode/HF/Mix: opto compression controls.
- Esser Threshold/Frequency/Range/Mode: sibilance control.

## Rack

- Rack In: input trim before module chain.
- Rack Out: output trim after module chain.
- Rack Mix: global dry/wet chain blend.

Implementation note: apply `setTooltip(...)` after each `setupSlider` / `setupBypassButton` once the current UI branch has passed build validation.
