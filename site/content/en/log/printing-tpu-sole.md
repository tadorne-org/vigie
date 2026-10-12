---
title: "Printing a TPU sole"
translationKey: log/printing-tpu-sole
date: 2026-10-10
description: "The enclosure sole and its pad, in 3D: the OpenSCAD source, the STL files and the TPU 95A Cura profiles are published."
params:
  viewer: true
  tags: ["vigie", "hardware", "enclosure", "3D printing"]
---

The [enclosure sole](/log/tpu-sole/) is printed in **TPU 95A**. Here are the
two parts, to rotate and slice with the mouse, and the settings that get them
out of the printer right. The sources are published in the
[`mecanique/`](https://github.com/tadorne-org/vigie/tree/main/mecanique) folder.

## The two parts

The **sole** lines the bottom of the enclosure. It carries the shunt in a
cradle whose lips close over the blocks, and the Vigie board on four posts.

{{< viewer src="/models/semelle_shunt_TPU.glb" label="Sole — shunt cradle and board posts" >}}

The **pad** is glued under the lid. It presses on the cradle with 1.5 mm of
crush: the shunt is clamped between the floor and the lid, with no play.

{{< viewer src="/models/tampon_shunt_TPU.glb" label="Pad — exported upside down, web against the bed" >}}

## The settings

Both Cura profiles start from the TPU 95A settings shared across Tadorne
projects: 195 °C (205 on the first layer), 0.15 mm layers, two 0.45 mm walls,
1.5 mm retraction at 20 mm/s with coasting, bed at 55 °C, 6 mm brim. They differ
only in infill.

| Part | Infill | Why |
|---|---|---|
| Sole | 25 % gyroid | the slab acts as a shock absorber |
| Pad | 100 % lines | stiff, to hold the preload |

The cradle must come out solid, and a Cura profile cannot do local infill. So a
**modifier** is loaded alongside the sole, forcing 100 % on the cradle, the lips
and the anchors: the slab stays gyroid, the rest is solid. Step-by-step
instructions are in the
[`mecanique/` README](https://github.com/tadorne-org/vigie/tree/main/mecanique#profils-dimpression-tpu-95a)
(in French).

The sole prints flat, underside on the bed; the pad is already flipped in its
file.

## To measure before printing

Several enclosure dimensions are still to be taken from the real part: inner
floor, corner posts, bolt head height, depth under the lid. They are marked
`>>> A MESURER` in the source. The published model is a starting point, not yet
a validated part.
