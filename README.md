# Vigie

**English** · [Français](README.fr.md)

**The battery's lookout.** An always-alive energy node for small sailboats: a
board that measures the energy on board, sleeps at a few microamps and reports
from the bilge — all year round, with the battery switch off. A
[Tadorne](https://tadorne.org/) project.

## The problem

A battery runs flat while the boat is left alone. The usual causes — an
instrument still wired live, a converter that never sleeps, no charging — are
invisible from the quay.

## The approach

- **A single node stays alive.** Powered upstream of the battery switch, it
  measures the whole installation and reports; everything else on board can be
  switched off or dead.
- **A single source of truth.** All the current goes through one shunt, read
  with four wires in the negative bus. No charging current flows through the
  board.
- **A standby that does not drain.** A converter chosen for its quiescent
  current: what a conventional converter would draw in a month, Vigie draws in
  a year.
- **A network, not a box.** Vigie is the gateway of a small radio network:
  displays, attitude unit, wind sensor and a second current sensor can join as
  needed, and any of them can fail without taking the rest down.

## Status

**Vigie 1.0**: board designed, assembled, first tests passed. The KiCad
project is published under CERN-OHL-S v2 and the 1.0.0 firmware under GPL-3.0.
Eight milestones, from the bench to a season afloat:
[roadmap](https://vigie.tadorne.org/roadmap/) ·
[technical documentation](https://vigie.tadorne.org/documentation/) ·
[log](https://vigie.tadorne.org/log/).

## Repository contents

| Folder | Contents | Licence |
|---|---|---|
| [`pcb/`](pcb/) | Vigie 1.0 KiCad project: schematic, four-layer board, symbol library | [CERN-OHL-S v2](pcb/LICENSE) |
| [`firmware/`](firmware/) | 1.0.0 firmware of the energy node (ESP32, PlatformIO), bench gateway and tools (documentation in French) | [GPL-3.0](firmware/LICENSE) |
| [`mecanique/`](mecanique/) | TPU 95A sole and pad for an IP67 sealed enclosure: OpenSCAD source, STL files, Cura profiles (documentation in French) | to be specified |
| [`site/`](site/) | Hugo source of [vigie.tadorne.org](https://vigie.tadorne.org/) | content [CC BY-SA 4.0](site/LICENSE-CONTENT), templates [MIT](site/LICENSE) |

The content of `pcb/` (tag `v1.0`) is identical to the published, signed and
timestamped archive:
[`vigie-1.0-kicad.zip`](https://vigie.tadorne.org/files/vigie-1.0/vigie-1.0-kicad.zip),
[signature](https://vigie.tadorne.org/files/vigie-1.0/vigie-1.0-kicad.zip.asc),
[FreeTSA timestamp](https://vigie.tadorne.org/files/vigie-1.0/vigie-1.0-kicad.zip.tsr).

## Authenticity

Commits and published files are signed with the Tadorne key
(`5420 08E3 53EC A628 B4D6  ACAD 8C03 243A FF69 5BF7`); every commit is
timestamped by FreeTSA (RFC 3161), the token stored in the Git notes
`refs/notes/freetsa`. Verification: <https://tadorne.org/verify/>.

```sh
git fetch origin refs/notes/freetsa:refs/notes/freetsa
git notes --ref freetsa show HEAD
```
