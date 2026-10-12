---
title: "Une semelle en TPU 95 pour le shunt et la carte"
translationKey: log/tpu-sole
date: 2026-10-10
description: "Le coffret reçoit sa semelle imprimée en TPU 95 : elle amortit les chocs du shunt et loge la carte."
params:
  tags: ["vigie", "matériel", "coffret"]
---

Le coffret du nœud a maintenant sa semelle, imprimée en **TPU 95**. Elle tient
deux choses à la fois : elle **amortit les chocs du shunt** — un bloc de métal
qui ne demande qu'à cogner contre la coque — et elle **loge la carte**, à
quelques millimètres du fond du coffret.

<figure>
  <img src="/img/v1-sole-print.webp" alt="Semelle en TPU orange en cours d'impression sur le plateau d'une imprimante Ender.">
</figure>

La pièce sort d'une imprimante de bureau, en filament souple. Sa face supérieure
est nervurée : la matière cède là où il faut, sans que la carte perde son
appui. Le reste du plateau se lit sur la photo — la buse a posé la première
couche sur toute la surface avant de monter.

<figure>
  <img src="/img/v1-sole.webp" alt="Semelle en TPU 95 terminée, face supérieure nervurée et logement du shunt, tenue à la main.">
</figure>

Le logement du shunt est taillé dans la même pièce. Le shunt s'y boulonne sur
ses deux bornes, et la carte se fixe par-dessus, sur ses quatre trous M3 : un
seul élément absorbe les vibrations et porte l'électronique, sans pièce
supplémentaire.

## Sur l'établi

Le montage complet tient dans le coffret : le shunt à gauche, la carte au
centre, la semelle entre les deux. Il reste le câblage à finir et les mesures
sur batterie.

<figure>
  <img src="/img/v1-enclosure.webp" alt="Le nœud monté dans son coffret : shunt à gauche, carte Vigie 1.0 au centre sur sa semelle en TPU, câbles de mesure à droite.">
</figure>

Le TPU 95 se choisit pour son élasticité : il encaisse ce qu'un support rigide
transmettrait à la carte, et il ne rouille pas au fond de la cale. Reste à
vérifier ce qu'il donne à l'usage, une saison à flot.

## Où

- [Documentation technique](/fr/documentation/) — la carte, la mesure, le
  micrologiciel.
- [Feuille de route](/fr/feuille-de-route/) — le coffret fait partie du jalon
  M4, « À flot ».
- [Le projet KiCad (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip) est publié sous
  CERN-OHL-S v2 ; le fichier de la semelle suivra avec le coffret.
