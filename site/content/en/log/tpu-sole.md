---
title: "A TPU 95 sole for the shunt and the board"
translationKey: log/tpu-sole
date: 2026-10-10
description: "The enclosure gets its printed TPU 95 sole: it cushions the shunt and houses the board."
params:
  tags: ["vigie", "hardware", "enclosure"]
---

The node's enclosure now has its sole, printed in **TPU 95**. It does two jobs
at once: it **cushions the shunt** — a block of metal only too happy to knock
against the hull — and it **houses the board**, a few millimetres off the
bottom.

<figure>
  <img src="/img/v1-sole-print.webp" alt="Orange TPU sole mid-print on the bed of an Ender printer.">
</figure>

The part comes off a desktop printer, in flexible filament. Its top face is
ribbed: the material gives where it should, without the board losing its
support. The rest of the build plate shows in the photo — the nozzle laid the
first layer across the whole surface before climbing.

<figure>
  <img src="/img/v1-sole.webp" alt="Finished TPU 95 sole, ribbed top face and shunt housing, held in hand.">
</figure>

The shunt's housing is cut into the same part. The shunt bolts into it on its
two terminals, and the board fastens on top, on its four M3 holes: a single
piece absorbs the vibration and carries the electronics, with no extra part
needed.

## On the bench

The whole assembly fits in the enclosure: shunt on the left, board in the
middle, sole between the two. Wiring is left to finish, and then the
measurements on a battery.

<figure>
  <img src="/img/v1-enclosure.webp" alt="The node mounted in its enclosure: shunt on the left, Vigie 1.0 board at centre on its TPU sole, measurement wires to the right.">
</figure>

TPU 95 is chosen for its elasticity: it takes what a rigid mount would pass on
to the board, and it does not rust at the bottom of the bilge. What is left is
to see how it behaves in service, a season afloat.

## Where

- [Technical documentation](/documentation/) — the board, the measurement, the
  firmware.
- [Roadmap](/roadmap/) — the enclosure is part of milestone M4, "Afloat".
- [The KiCad project (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip) is published
  under CERN-OHL-S v2; the sole's file will follow with the enclosure.
