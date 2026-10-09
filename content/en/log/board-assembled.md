---
title: "Board assembled, first tests pass"
translationKey: log/board-assembled
date: 2026-10-07
description: "Components are on the first board; the microcontroller, the INA226 and the admin page all answer. The 12 V battery is next."
params:
  tags: ["vigie", "hardware", "firmware"]
---

The components are on the board. The first board of the batch is assembled —
paste through the stencil, reflow on a hotplate, then the through-hole parts by
iron — and it survived its first power-up.

<figure>
  <img src="/img/v1-assembled.webp" alt="Assembled Vigie 1.0 board, powered over USB-C, on the bench.">
</figure>

## What answered

- **3.3 V is there.** The green LED lights up; the board breaks its silence.
- **USB-C and the CH340C do their job.** The board flashes like a devkit, with
  no external probe or programmer.
- **The firmware runs.** The ESP32 opens its access point, serves its admin
  page and reports its version number.
- **The INA226 answers on I2C.** The "Sensor" line reads `INA226 (continuous)`
  rather than the simulator: the measurement chain is alive.

<figure>
  <img src="/img/v1-dashboard.webp" alt="Vigie 1.0 dashboard, served from the board's own access point.">
</figure>

Powered from USB and with no shunt, the current columns mean nothing yet: the
**109.23 A** on screen is the INA226's full scale (81.92 mV across a 0.75 mΩ
shunt), with the input left floating. The battery is what will give those
numbers a meaning.

## Next: the 12 V battery

Next step, measurements on a battery: the **real quiescent current**, the one
that sets the winter autonomy, the **current sign at the shunt**, and
calibration against a reference. The bench then closes milestone M1, and
bring-up closes M2 with it.

## Where

- [The Design page](/design/) recalls how the board is produced.
- [Roadmap](/roadmap/) — milestones M1 and M2.
- [KiCad project (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip), published under
  CERN-OHL-S v2.
