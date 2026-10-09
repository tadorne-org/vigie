---
title: "The bare boards have arrived"
translationKey: log/bare-boards-arrived
date: 2026-10-02
description: "Vigie 1.0's printed circuit boards are back from the fab, stencil included."
params:
  tags: ["vigie", "hardware", "fabrication"]
---

Vigie 1.0's bare boards are in. Four-layer, 86 × 54 mm, with the paste stencil
from the same order.

<figure>
  <img src="/img/v1-boards-arrived.webp" alt="Bare Vigie 1.0 boards: outline, silkscreen and Tadorne logo.">
</figure>

## What came back

The board holds what the file promised. Outline, four M3 holes and both inner
planes are there, and the silkscreen carries enough to wire it without the
schematic at hand: 12 V input and shunt (J1, J2), USB-C and the power jumper,
both expansion headers (J7, J8), plus the **Vigie 1.0 — by Tadorne** logo.

## What is left

Assembly happens on the bench: paste through the stencil, reflow on a hotplate,
then the through-hole parts by iron. One board goes first; the others follow
once it is validated.

Then comes bring-up, with the two measurements that decide the rest: the real
quiescent current, which sets the winter autonomy, and the current sign at the
shunt. The design's open points stay open — antenna in a closed locker, case
clearance, USB-C receptacle overhang.

## Where

- [The Design page](/design/) recalls how the board is produced.
- [KiCad project (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip), published under
  CERN-OHL-S v2.
- [Roadmap](/roadmap/) — milestone M2, first boards.
