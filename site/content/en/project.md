---
title: "The project"
translationKey: project
url: /project/
description: "One board that measures the boat's energy, all year round."
params:
  eyebrow: "Vigie"
---

## The problem

A battery goes flat while the boat is left alone. The usual causes — an instrument still wired live, a converter that never sleeps, no recharge at all — are invisible from the quay.

## The approach

One node, fed upstream of the main switch, is the only thing that stays alive: it measures the whole installation and reports. Everything else on board may be switched off or broken; the watch goes on.

## One point of truth

All current passes through a single shunt, in the negative bus. The board reads the voltage across it in four wires — two for the load, two for the measure. No charge flows through the board itself, so nothing sits in parallel to falsify the count.

## The quiet drain

A watch is only useful if it does not itself empty the battery. Conversion is chosen for its quiescent current, counted in microamps rather than milliamps: what a conventional converter would take in a month, Vigie takes in a year.

## A network, not a box

Vigie is the always-on gateway of a small radio network. Displays, an attitude sensor, a wind sensor, a second current sensor: each joins when needed, and each may fail without taking the rest down.

## The horizon

The design — schematic, placement, routing — is produced by versioned scripts and published as a commons. The next steps are a second current sensor for the outboard, an attitude node and a wind sensor at the masthead.
