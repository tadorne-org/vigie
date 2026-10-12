---
title: "Firmware 1.0: the node's code is published"
translationKey: log/firmware-1-0
date: 2026-10-12
description: "Vigie's firmware reaches 1.0.0 and joins the public repository under GPL-3.0: measurement, SOC, logging, ESP-NOW, BLE and an admin page."
params:
  tags: ["vigie", "firmware", "release"]
---

The energy node's firmware is published as **version 1.0.0**, under the
**GPL-3.0** licence, in the
[tadorne-org/vigie](https://github.com/tadorne-org/vigie/tree/main/firmware)
repository, next to the board (CERN-OHL-S) and the enclosure sole.

<figure>
  <img src="/img/v1-dashboard.webp" alt="Vigie's admin page, served by the board itself and opened on an iPhone.">
  <figcaption>The admin page, served by the board itself. Captured with version
  0.1.0, on the bare board with no battery: the values and alarms shown are
  meaningless, the interface is what matters.</figcaption>
</figure>

## What it does

- **Measurement**: INA226 in continuous mode, current computed from the raw
  shunt voltage, with gain, offset and sign adjustable without recompiling.
- **State of charge** by coulomb counting, with charge efficiency and Peukert
  correction.
- **Deep sleep**: a sleep cycle, a measurement every 5 min, a flash record that
  survives restarts, a push every 15 min.
- **Radio**: ESP-NOW (TsubameBus protocol, optional encryption) and
  **BTHome v2** Bluetooth advertising, which Home Assistant reads as is.
- **Battery protection**: throttle, survival and cut-off modes, with alerts.
- **Admin page**: a "Vigie-XXXX" Wi-Fi access point at power-up or on a BOOT
  press, with dashboard, history and settings; the same over the serial command
  line.
- **Simulator**: on a bare ESP32 devkit, a battery simulator stands in for the
  sensor, and every simulated measurement carries a `SIM` flag.

It builds with PlatformIO, with no external library, for the Vigie 1.0 board,
a devkit or the bench gateway.

## What is left to verify

1.0 does not mean everything has been measured. The
[firmware README](https://github.com/tadorne-org/vigie/tree/main/firmware)
keeps the list (in French), and it is an honest one:

- **real reception** of ESP-NOW frames and of the BLE advertisement, which needs
  a second ESP32 and an authorised phone;
- everything involving the INA226 under **real current**: alert, calibration,
  sign;
- the node's **idle current**, estimated today and not measured;
- **clock drift** during sleep, since there is no 32 kHz crystal;
- downlink commands, whose receive structure exists but not the commands.

The coming weeks are for closing them, with a battery.
