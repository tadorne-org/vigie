---
title: "Impression d'une semelle TPU"
translationKey: log/printing-tpu-sole
date: 2026-10-10
description: "La semelle du coffret et son tampon se pilotent en 3D : le fichier OpenSCAD, les STL et les profils Cura TPU 95A sont publiés."
params:
  viewer: true
  tags: ["vigie", "matériel", "coffret", "impression 3D"]
---

La [semelle du coffret](/fr/journal/semelle-tpu/) s'imprime en **TPU 95A**. Voici
les deux pièces, à tourner et à couper à la souris, et les réglages qui les
font sortir correctement. Les sources sont publiées dans le dossier
[`mecanique/`](https://github.com/tadorne-org/vigie/tree/main/mecanique).

## Les deux pièces

La **semelle** tapisse le fond du coffret. Elle porte le shunt dans un berceau
dont les lèvres se referment sur les blocs, et la carte Vigie sur quatre
colonnettes.

{{< viewer src="/models/semelle_shunt_TPU.glb" label="Semelle — berceau du shunt et colonnettes de la carte" >}}

Le **tampon** se colle sous le couvercle. Il appuie sur le berceau avec 1,5 mm
d'écrasement : le shunt est serré entre le fond et le couvercle, sans jeu.

{{< viewer src="/models/tampon_shunt_TPU.glb" label="Tampon — exporté retourné, l'âme contre le plateau" >}}

## Les réglages

Les deux profils Cura partent des réglages TPU 95A communs aux projets
Tadorne : 195 °C (205 à la première couche), couches de 0,15 mm, deux parois de
0,45 mm, rétraction de 1,5 mm à 20 mm/s avec coasting, plateau à 55 °C, bordure
de 6 mm. Ils ne diffèrent que par le remplissage.

| Pièce | Remplissage | Pourquoi |
|---|---|---|
| Semelle | 25 % gyroïde | la dalle sert d'amortisseur |
| Tampon | 100 % en lignes | raide, pour tenir la précontrainte |

Le berceau, lui, doit sortir plein, et un profil Cura ne sait pas faire de
remplissage local. On charge donc avec la semelle un **modificateur** qui
impose 100 % sur le berceau, les lèvres et les ancrages : la dalle reste en
gyroïde, le reste est massif. Le mode d'emploi pas à pas est dans le
[README de `mecanique/`](https://github.com/tadorne-org/vigie/tree/main/mecanique#profils-dimpression-tpu-95a).

La semelle s'imprime à plat, le dessous contre le plateau ; le tampon est déjà
retourné dans son fichier.

## À mesurer avant d'imprimer

Plusieurs cotes du coffret sont encore à relever sur la pièce réelle : fond
intérieur, colonnes d'angle, hauteur des têtes de boulon, profondeur sous le
couvercle. Elles sont marquées `>>> A MESURER` dans le source. Le modèle publié
est un point de départ, pas encore une pièce validée.
