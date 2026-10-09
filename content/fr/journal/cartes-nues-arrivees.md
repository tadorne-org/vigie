---
title: "Les cartes nues sont arrivées"
translationKey: log/bare-boards-arrived
date: 2026-10-02
description: "Les circuits imprimés de Vigie 1.0 sont sortis de fonderie, pochoir compris."
params:
  tags: ["vigie", "matériel", "fabrication"]
---

Le lot de cartes nues de **Vigie 1.0** est arrivé. Circuits imprimés quatre
couches de 86 × 54 mm, avec le pochoir de dépôt de pâte qui faisait partie de
la même commande.

<figure>
  <img src="/img/v1-boards-arrived.webp" alt="Cartes nues Vigie 1.0, contour, sérigraphie et logo Tadorne.">
</figure>

## Ce que la fonderie a rendu

La carte tient ce que le fichier annonçait. Le contour, les quatre trous M3 et
les deux plans internes sont au rendez-vous, et la sérigraphie porte de quoi
câbler sans avoir le schéma sous les yeux : entrée 12 V et shunt (J1, J2),
USB-C et cavalier d'alimentation, les deux barrettes d'extension (J7, J8),
plus le logo **Vigie 1.0 — by Tadorne**.

## Ce qui reste à faire

L'assemblage se fera à l'établi : pâte au pochoir, refusion sur plaque, puis
les traversants au fer. Une seule carte passe en premier ; les autres suivent
une fois celle-là validée.

Vient ensuite la mise en service, avec les deux mesures qui décident de la
suite : le courant de repos réel, qui fixe l'autonomie hivernale, et le signe
du courant au shunt. Les points ouverts du dossier restent ouverts — antenne en
fond de cale, tenue du coffret, recul de l'embase USB-C.

## Où

- [La page Conception](/fr/conception/) rappelle comment la carte est produite.
- [Projet KiCad (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip), publié sous
  CERN-OHL-S v2.
- [Feuille de route](/fr/feuille-de-route/) — jalon M2, premières cartes.
