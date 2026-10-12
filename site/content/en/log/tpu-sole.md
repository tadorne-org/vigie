---
title: "A TPU 95 sole for the shunt and the board"
translationKey: log/tpu-sole
date: 2026-10-10
description: "The enclosure gets its printed TPU sole: it lines the bottom, cradles the shunt and carries the board."
params:
  tags: ["vigie", "hardware", "enclosure"]
---

The enclosure now has its sole, printed in **TPU 95A**. It is a single flexible
piece that lines the whole bottom: it **cradles the shunt** — a block of metal
only too happy to knock against the hull — and it **carries the board**, a few
millimetres above the bottom, on four posts with M3 screws.

<figure>
  <img src="/img/v1-sole-print.webp" alt="Orange TPU sole mid-print on the bed of an Ender printer.">
</figure>

The shunt is pressed down into its cradle from above: the lips spread, the TPU
gives, then they close again over the blocks. The pocket is slightly shorter
than the shunt, which stays pinched along its length. Laid in the bottom, the
sole touches all four walls and the corner columns: once screwed down it cannot
move, and the cradle can no longer open. The shunt is captive.

<figure>
  <img src="/img/v1-sole.webp" alt="Finished TPU 95A sole held in hand: flat underside, shunt cradle along one edge.">
</figure>

The underside is flat, with no recess at all: the bottom of the enclosure is
covered in moulded studs, to be cut flush. Two M4 screws and compression
sleeves cross the slab — the screw tightens metal onto a boss, and the TPU,
which creeps, stays preloaded. In front of each end of the cradle, an anchor
holds the 16 mm² cable: its weight no longer levers on the M8 bolt.

## What absorbs the shock

The slab prints at **25% gyroid**: the infill itself acts as a spring. The
cradle, the lips and the anchors print solid. The rest of the hardening is a
solid buffer bonded under the lid: it presses on the top of the cradle with
1.5 mm of preload. The shunt is then clamped between the bottom and the lid,
with no play — the lips can no longer open under a vertical shock.

## Printing it

Flexible filament on an Ender 3: 0.15 mm layers, 0.3 mm for the first one,
25 mm/s, nozzle at 195 °C, bed at 55 °C, a brim for adhesion and no supports.

## On the bench

The assembly fits in the enclosure: shunt on the left, board at centre, above
its sole. Wiring is left to finish, and then the measurements on a battery.

<figure>
  <img src="/img/v1-enclosure.webp" alt="The node mounted in its enclosure: shunt on the left, Vigie 1.0 board at centre above its TPU sole, measurement wires in the foreground.">
</figure>

TPU 95A is chosen for its elasticity: it takes what a rigid mount would pass on
to the board, and it does not rust at the bottom of the bilge. What is left is
to see how it behaves in service, a season afloat.

## Where

- [Technical documentation](/documentation/) — the board, the measurement, the
  firmware.
- [Roadmap](/roadmap/) — the enclosure is part of milestone M4, "Afloat".
- [The KiCad project (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip) is published
  under CERN-OHL-S v2; the sole's sources will follow.
