---
title: "Une semelle en TPU 95 pour le shunt et la carte"
translationKey: log/tpu-sole
date: 2026-10-10
description: "Le coffret reçoit sa semelle imprimée en TPU : elle tapisse le fond, berce le shunt et porte la carte."
params:
  tags: ["vigie", "matériel", "coffret"]
---

Le coffret a maintenant sa semelle, imprimée en **TPU 95A**. C'est une seule
pièce souple qui tapisse tout le fond : elle **berce le shunt** — un bloc de
métal qui ne demande qu'à cogner contre la coque — et elle **porte la carte**,
à quelques millimètres au-dessus du fond, sur quatre colonnettes à vis M3.

<figure>
  <img src="/img/v1-sole-print.webp" alt="Semelle en TPU orange en cours d'impression sur le plateau d'une imprimante Ender.">
</figure>

Le shunt s'enfonce dans son berceau par le dessus : les lèvres s'écartent, le
TPU cède, puis elles se referment sur les blocs. La poche est un peu plus
courte que le shunt, qui reste pincé en longueur. Posée au fond, la semelle
touche les quatre parois et les colonnes d'angle : une fois vissée, elle ne
bouge plus et le berceau ne peut plus s'ouvrir. Le shunt est prisonnier.

<figure>
  <img src="/img/v1-sole.webp" alt="Semelle en TPU 95A terminée, tenue à la main : dessous plat, berceau du shunt sur le bord.">
</figure>

Le dessous est plat, sans aucun logement : le fond du coffret est garni de
picots moulés, à couper au ras. Deux vis M4 et des bagues de compression
traversent la dalle — la vis serre du métal sur un bossage, et le TPU, qui
flue, reste précontraint. Devant chaque about du berceau, un ancrage tient le
câble de 16 mm² : sa masse ne fait plus levier sur le boulon M8.

## Ce qui amortit

La dalle s'imprime à **25 % de gyroïde** : le remplissage fait office de
ressort. Le berceau, les lèvres et les ancrages, eux, s'impriment pleins. Le
reste du durcissement est un tampon imprimé plein et collé sous le couvercle :
il appuie sur le sommet du berceau avec 1,5 mm de précontrainte. Le shunt est
alors serré entre le fond et le couvercle, sans jeu — les lèvres ne peuvent
plus s'ouvrir sous un choc vertical.

## À l'impression

Filament souple sur une Ender 3 : couches de 0,15 mm, 0,3 mm pour la première,
25 mm/s, buse à 195 °C, plateau à 55 °C, bordure d'accroche et pas de support.

## Sur l'établi

Le montage tient dans le coffret : le shunt à gauche, la carte au centre,
au-dessus de sa semelle. Il reste le câblage à finir et les mesures sur
batterie.

<figure>
  <img src="/img/v1-enclosure.webp" alt="Le nœud monté dans son coffret : shunt à gauche, carte Vigie 1.0 au centre au-dessus de sa semelle en TPU, câbles de mesure au premier plan.">
</figure>

Le TPU 95A se choisit pour son élasticité : il encaisse ce qu'un support rigide
transmettrait à la carte, et il ne rouille pas au fond de la cale. Reste à
vérifier ce qu'il donne à l'usage, une saison à flot.

## Où

- [Documentation technique](/fr/documentation/) — la carte, la mesure, le
  micrologiciel.
- [Feuille de route](/fr/feuille-de-route/) — le coffret fait partie du jalon
  M4, « À flot ».
- [Le projet KiCad (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip) est publié sous
  CERN-OHL-S v2 ; les sources de la semelle suivront.
