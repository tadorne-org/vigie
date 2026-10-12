---
title: "A TPU 95 sole for the shunt and the board"
translationKey: log/tpu-sole
date: 2026-10-10
description: "The enclosure's sole is printed: it cradles the shunt and carries the board."
params:
  tags: ["vigie", "hardware", "enclosure"]
---

The enclosure's sole is printed. A single piece in **TPU 95A**, flexible,
printed flat on an Ender 3: a 6 mm slab that lines the whole bottom of the
enclosure.

<figure>
  <img src="/img/v1-sole-print.webp" alt="Orange TPU sole mid-print on the bed of an Ender printer.">
</figure>

## What the sole does

It carries the two parts that must not move: it **cradles the shunt** in a
cradle whose lips close over the blocks, and it **carries the board** on four
posts with M3 screws. The pocket is slightly shorter than the shunt, which
stays pinched along its length; the underside is flat, for an enclosure bottom
covered in moulded studs.

<figure>
  <img src="/img/v1-sole.webp" alt="Finished TPU 95A sole held in hand: flat underside, shunt cradle along one edge.">
</figure>

A solid buffer, bonded under the lid, presses on the cradle with 1.5 mm of
preload: the shunt is clamped between the bottom and the lid, with no play. The
slab prints in 25% gyroid, the cradle solid; two M4 screws and compression
sleeves press the whole thing to the bottom.

## On the bench

The node is mounted in its enclosure: shunt on the left, board at centre, above
its sole.

<figure>
  <img src="/img/v1-enclosure.webp" alt="The node mounted in its enclosure: shunt on the left, Vigie 1.0 board at centre above its TPU sole.">
</figure>

## Next: wiring and measurements

Wiring is left to finish, then the measurements on a battery — the **real
quiescent current** and the **current sign at the shunt**. The bench then closes
milestone M1.

## Where

- [Technical documentation](/documentation/) — the board, the measurement, the
  firmware.
- [Roadmap](/roadmap/) — milestones M1 and M4.
