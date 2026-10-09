---
title: "Le projet"
translationKey: project
url: /fr/projet/
description: "Une carte qui mesure l'énergie du bord, toute l'année."
params:
  eyebrow: "Vigie"
---

## Le problème

Une batterie se vide pendant que le bateau est laissé seul. Les causes habituelles — un instrument resté câblé en direct, un convertisseur qui ne dort jamais, aucune recharge — sont invisibles depuis le quai.

## La démarche

Un nœud, alimenté en amont du coupe-circuit, est la seule chose qui reste vivante : il mesure toute l'installation et rend compte. Tout le reste du bord peut être éteint ou en panne ; la veille continue.

## Un seul point de vérité

Tout le courant passe par un shunt unique, dans le bus négatif. La carte lit la tension à ses bornes en quatre fils — deux pour la charge, deux pour la mesure. Aucun courant de charge ne traverse la carte : rien ne se met en parallèle pour fausser le compte.

## La veille qui ne vide pas

Une veille n'est utile que si elle ne vide pas elle-même la batterie. La conversion est choisie pour son courant de repos, compté en microampères plutôt qu'en milliampères : ce qu'un convertisseur classique prendrait en un mois, Vigie le prend en un an.

## Un réseau, pas un boîtier

Vigie est la passerelle toujours vivante d'un petit réseau radio. Afficheurs, centrale d'attitude, capteur de vent, second capteur de courant : chacun rejoint le réseau au besoin, et chacun peut tomber sans emporter le reste.

## L'horizon

La conception — schéma, implantation, routage — est produite par des scripts versionnés et publiée comme un commun. Les prochaines étapes : un second capteur de courant pour le hors-bord, une centrale d'attitude et un capteur de vent en tête de mât.
