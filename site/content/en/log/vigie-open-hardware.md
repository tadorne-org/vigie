---
title: "Vigie goes open hardware"
translationKey: log/vigie-open-hardware
date: 2026-09-28
description: "Vigie 1.0's schematic and four-layer board are published under CERN-OHL-S v2."
params:
  tags: ["vigie", "hardware", "licence"]
---

Vigie 1.0's KiCad project is published under the **CERN-OHL-S v2** hardware
licence. Schematic, four-layer board and symbol library: the minimum needed to
open, study and rebuild the board is downloadable now.

## What is published

| Item | Format |
|---|---|
| Schematic | KiCad |
| Printed circuit board | KiCad, four layers, routed |
| Library | the project's `CH340C_3V3` symbol |

The project opens as-is in KiCad 10. The bill of materials is not supplied
separately: it exports straight from the schematic, with KiCad's own tools.

## Responsibility and limits

- **The board is untested.** Integration testing is under way, and the real
  quiescent current is not measured yet. The design is published as it stands,
  open points included — antenna in a closed locker, shunt orientation to be
  confirmed.
- **No warranty.** The licence text is explicit: the source is provided as-is.
  A board switching 12 V aboard involves the safety of whoever wires it.
- **Firmware is still to be published.** This archive covers the hardware; the
  code that reads the INA226 and keeps watch through the winter will follow
  under its own licence.

## Where

- [KiCad project (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip)
- [Detached signature (`.asc`)](/files/vigie-1.0/vigie-1.0-kicad.zip.asc)
- [FreeTSA timestamp (`.tsr`)](/files/vigie-1.0/vigie-1.0-kicad.zip.tsr)
- [CERN-OHL-S v2 licence text](/files/vigie-1.0/LICENSE-CERN-OHL-S-2.0.txt)

The archive is signed with the Tadorne publication key; the fingerprint is in
the page footer. It is also timestamped by FreeTSA (RFC 3161).

The published source is authoritative; it is also described on [the Design
page](/design/), which recalls how the board is produced.
