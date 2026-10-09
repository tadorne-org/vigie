---
title: "La conception"
translationKey: design
url: /fr/conception/
description: "Schéma, implantation et routage, régénérés par des scripts versionnés."
---

Vigie n'est pas dessinée à la main. Son schéma, l'implantation des empreintes et le routage sont produits par des scripts conservés avec la conception : la même commande reconstruit les mêmes fichiers.

- Le schéma provient d'une description unique des composants et des nets.
- Contour de carte, trous de fixation, implantation des empreintes et plans internes de masse et de 3,3 V sont scriptés.
- Le routage des signaux est confié à un routeur, sur les deux couches externes seulement ; les couches internes restent des plans pleins.
- Chaque build se termine par un contrôle de la netlist broche par broche, un contrôle des règles électriques et un contrôle des règles de dessin.

{{< callout title="Carte" >}}
86 × 54 mm, quatre couches, soixante-neuf empreintes, cinquante-quatre nets. Mesure aux bornes d'un shunt externe de 100 A, en quatre fils.
{{< /callout >}}

Le projet KiCad est publié sous [CERN-OHL-S v2](/files/vigie-1.0/LICENSE-CERN-OHL-S-2.0.txt) : [archive](/files/vigie-1.0/vigie-1.0-kicad.zip), sa [signature](/files/vigie-1.0/vigie-1.0-kicad.zip.asc) et son [horodatage FreeTSA](/files/vigie-1.0/vigie-1.0-kicad.zip.tsr). La nomenclature s'exporte du schéma ; la procédure d'étalonnage suivra avec le micrologiciel.
