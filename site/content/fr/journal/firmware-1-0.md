---
title: "Firmware 1.0 : le code du nœud est publié"
translationKey: log/firmware-1-0
date: 2026-10-12
description: "Le firmware de Vigie passe en 1.0.0 et rejoint le dépôt public, sous GPL-3.0 : mesure, SOC, journal, ESP-NOW, BLE et page d'administration."
params:
  tags: ["vigie", "micrologiciel", "publication"]
---

Le firmware du nœud énergie est publié en **version 1.0.0**, sous licence
**GPL-3.0**, dans le dépôt
[tadorne-org/vigie](https://github.com/tadorne-org/vigie/tree/main/firmware),
à côté de la carte (CERN-OHL-S) et de la semelle du coffret.

<figure>
  <img src="/img/v1-dashboard.webp" alt="Page d'administration de Vigie, servie par la carte elle-même et ouverte sur un iPhone.">
  <figcaption>La page d'administration, servie par la carte elle-même. Capture prise
  avec la version 0.1.0, sur la carte nue, sans batterie : les valeurs et les
  alarmes affichées n'ont aucun sens, c'est l'interface qui compte.</figcaption>
</figure>

## Ce qu'il fait

- **Mesure** : INA226 en continu, courant calculé depuis la tension de shunt
  brute, avec gain, zéro et sens réglables sans recompiler.
- **État de charge** par comptage coulométrique, avec rendement de charge et
  correction de Peukert.
- **Veille profonde** : un cycle de sommeil, une mesure toutes les 5 min, un
  enregistrement en mémoire flash qui survit aux redémarrages, un envoi toutes
  les 15 min.
- **Radio** : ESP-NOW (protocole TsubameBus, chiffrement en option) et annonce
  Bluetooth **BTHome v2**, que Home Assistant lit telle quelle.
- **Protection de la batterie** : modes réduit, survie et coupure, avec alertes.
- **Page d'administration** : un point d'accès Wi-Fi « Vigie-XXXX » à la mise
  sous tension ou à l'appui sur BOOT, avec tableau de bord, historique et
  réglages ; la même chose en ligne de commande série.
- **Simulateur** : sur une devkit ESP32 nue, un simulateur de batterie remplace
  le capteur, et chaque mesure simulée porte un drapeau `SIM`.

Il se compile avec PlatformIO, sans bibliothèque externe, pour la carte
Vigie 1.0, pour une devkit ou pour la passerelle de banc.

## Ce qui reste à vérifier

Une 1.0 ne veut pas dire que tout est mesuré. Le
[README du firmware](https://github.com/tadorne-org/vigie/tree/main/firmware)
tient la liste, et elle est honnête :

- la **réception réelle** des trames ESP-NOW et de l'annonce BLE, qui demande
  un second ESP32 et un téléphone autorisé ;
- tout ce qui touche à l'INA226 sous **courant réel** : alerte, calibration,
  signe ;
- le **courant de repos** du nœud, aujourd'hui estimé et non mesuré ;
- la **dérive de l'horloge** en sommeil, qui n'a pas de quartz 32 kHz ;
- les commandes descendantes, dont la structure de réception existe mais pas
  les commandes.

Les prochaines semaines servent à les fermer, batterie à l'appui.
