---
title: "Carte montée, premiers tests concluants"
translationKey: log/board-assembled
date: 2026-10-07
description: "Les composants sont montés sur la première carte ; le microcontrôleur, l'INA226 et la page d'administration répondent. Reste la batterie 12 V."
params:
  tags: ["vigie", "matériel", "micrologiciel"]
---

Les composants sont montés sur la carte. La première carte du lot est assemblée
— pâte au pochoir, refusion sur plaque, puis les traversants au fer — et elle a
tenu sa première mise sous tension.

<figure>
  <img src="/img/v1-assembled.webp" alt="Carte Vigie 1.0 assemblée, alimentée par l'USB-C, sur le banc.">
</figure>

## Ce qui a répondu

- **Le 3,3 V est là.** La LED verte s'allume, la carte sort de son silence.
- **L'USB-C et le CH340C font leur travail.** La carte se flashe comme une
  devkit, sans sonde ni programmateur externe.
- **Le micrologiciel tourne.** L'ESP32 ouvre son point d'accès, sert sa page
  d'administration et affiche son numéro de version.
- **L'INA226 répond sur l'I2C.** La ligne « Capteur » annonce `INA226
  (continu)` et non le simulateur : la chaîne de mesure est vivante.

<figure>
  <img src="/img/v1-dashboard.webp" alt="Tableau de bord de Vigie 1.0, servi depuis le point d'accès de la carte.">
</figure>

Alimentée par l'USB et sans shunt, les colonnes de courant ne veulent encore
rien dire : les **109,23 A** affichés sont le fond d'échelle de l'INA226
(81,92 mV sur un shunt de 0,75 mΩ), entrée laissée en l'air. C'est la batterie
qui donnera un sens à ces chiffres.

## La suite : la batterie 12 V

Prochaine étape, les mesures sur batterie : le **courant de repos réel**, celui
qui fixe l'autonomie hivernale, le **sens du courant au shunt**, et
l'étalonnage face à une référence. Le banc ferme alors le jalon M1, et la mise
en service le M2 avec lui.

## Où

- [La page Conception](/fr/conception/) rappelle comment la carte est produite.
- [Feuille de route](/fr/feuille-de-route/) — jalons M1 et M2.
- [Projet KiCad (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip), publié sous
  CERN-OHL-S v2.
