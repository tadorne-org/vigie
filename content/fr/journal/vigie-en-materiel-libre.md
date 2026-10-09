---
title: "Vigie passe en matériel libre"
translationKey: log/vigie-open-hardware
date: 2026-09-28
description: "Le schéma et la carte quatre couches de Vigie 1.0 sont publiés sous CERN-OHL-S v2."
params:
  tags: ["vigie", "matériel", "licence"]
---

Le projet KiCad de **Vigie 1.0** est publié sous la licence matérielle
**CERN-OHL-S v2**. Schéma, carte quatre couches et bibliothèque de symboles : le
minimum nécessaire pour ouvrir, étudier et refabriquer la carte est
téléchargeable dès maintenant.

## Ce qui est publié

| Élément | Format |
|---|---|
| Schéma | KiCad |
| Circuit imprimé | KiCad, quatre couches, routé |
| Bibliothèque | symbole `CH340C_3V3` du projet |

Le projet s'ouvre tel quel dans KiCad 10. La nomenclature n'est pas fournie à
part : elle s'exporte directement du schéma, avec les outils de KiCad.

## Responsabilité et limites

- **La carte n'est pas éprouvée.** Les tests d'intégration sont en cours, et le
  courant de repos réel n'est pas encore mesuré. Le dossier est publié dans son
  état, points ouverts compris — antenne en fond de cale, orientation du shunt
  à valider.
- **Aucune garantie.** Le texte de la licence est explicite : la source est
  fournie telle quelle. Une carte qui commute 12 V à bord engage la sécurité de
  celui qui la câble.
- **Le micrologiciel reste à publier.** Cette archive couvre le matériel ; le
  code qui lit l'INA226 et tient l'hivernage suivra sa propre licence.

## Où

- [Projet KiCad (ZIP)](/files/vigie-1.0/vigie-1.0-kicad.zip)
- [Signature détachée (`.asc`)](/files/vigie-1.0/vigie-1.0-kicad.zip.asc)
- [Horodatage FreeTSA (`.tsr`)](/files/vigie-1.0/vigie-1.0-kicad.zip.tsr)
- [Texte de la licence CERN-OHL-S v2](/files/vigie-1.0/LICENSE-CERN-OHL-S-2.0.txt)

L'archive est signée avec la clé de publication Tadorne ; l'empreinte figure
dans le pied de page. Elle est aussi horodatée par FreeTSA (RFC 3161).

La source publiée fait autorité ; elle est aussi décrite sur [la page
Conception](/fr/conception/), qui rappelle comment la carte est produite.
