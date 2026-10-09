---
title: "Vigie"
translationKey: home
description: "Un nœud d'énergie toujours vivant pour petits voiliers : une carte qui mesure la batterie, veille tout l'hiver et rend compte depuis la cale. Un projet Tadorne."
params:
  hero:
    eyebrow: "Un projet Tadorne"
    lead: "La vigie de la batterie. Une carte qui mesure l'énergie à bord, veille à quelques microampères et rend compte depuis la cale — toute l'année, coupe-circuit ouvert."
    ctas:
      - label: "Le projet"
        url: "/fr/projet/"
      - label: "La conception"
        url: "/fr/conception/"
        solid: true
  video:
    mp4: "/video/vigie-fr.mp4"
    webm: "/video/vigie-fr.webm"
    poster: "/video/vigie-fr-poster.jpg"
    caption: "Vigie 1.0 : la vue éclatée, puis la carte à bord — shunt, paire Kelvin, Bluetooth LE."
  figures:
    - label: "Carte"
      value: "86 × 54 mm"
    - label: "Couches"
      value: "4"
    - label: "Courant de veille"
      value: "22 µA"
    - label: "Courant mesuré"
      value: "100 A"
    - label: "Régime"
      value: "Toute l'année"
  notes:
    - title: "Un seul point de vérité"
      body: "Tout le courant passe par le shunt ; la carte ne fait que le lire. Aucun chemin parallèle, aucune erreur silencieuse."
    - title: "Toujours vivante"
      body: "Alimentée en amont du coupe-circuit, c'est le seul module qui passe l'hiver."
    - title: "Fabriquée par scripts"
      body: "Schéma, implantation et routage sont régénérés par du code versionné, pas dessinés à la main."
  closing:
    title: "Lire la conception"
    body: "Schéma, implantation et routage, produits par des scripts versionnés."
    url: "/fr/conception/"
    label: "Le dossier de conception"
---

L'état de la batterie lu au shunt, et une veille tenue toute l'année.
