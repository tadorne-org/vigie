# Vigie

**La vigie de la batterie.** Un nœud d'énergie toujours vivant pour petits
voiliers : une carte qui mesure l'énergie à bord, veille à quelques
microampères et rend compte depuis la cale — toute l'année, coupe-circuit
ouvert. Un projet [Tadorne](https://tadorne.org/).

## Le problème

Une batterie se vide pendant que le bateau est laissé seul. Les causes
habituelles — un instrument resté câblé en direct, un convertisseur qui ne
dort jamais, aucune recharge — sont invisibles depuis le quai.

## L'approche

- **Un seul nœud reste vivant.** Alimenté en amont du coupe-circuit, il mesure
  toute l'installation et rend compte ; tout le reste du bord peut être éteint
  ou en panne.
- **Un seul point de vérité.** Tout le courant passe par un shunt unique, lu
  en quatre fils dans le bus négatif. Aucun courant de charge ne traverse la
  carte.
- **Une veille qui ne vide pas.** Conversion choisie pour son courant de
  repos : ce qu'un convertisseur classique prendrait en un mois, Vigie le
  prend en un an.
- **Un réseau, pas un boîtier.** Vigie est la passerelle d'un petit réseau
  radio : afficheurs, centrale d'attitude, capteur de vent, second capteur de
  courant rejoignent au besoin, et chacun peut tomber sans emporter le reste.

## État

**Vigie 1.0** : carte conçue, assemblée, premiers tests concluants. Le projet
KiCad est publié sous CERN-OHL-S v2 ; le micrologiciel suivra, sous sa propre
licence. Huit jalons, du banc à une saison à flot :
[feuille de route](https://vigie.tadorne.org/fr/feuille-de-route/) ·
[documentation technique](https://vigie.tadorne.org/fr/documentation/) ·
[journal](https://vigie.tadorne.org/fr/journal/).

## Contenu du dépôt

| Dossier | Contenu | Licence |
|---|---|---|
| [`pcb/`](pcb/) | Projet KiCad de Vigie 1.0 : schéma, carte quatre couches, bibliothèque de symboles | [CERN-OHL-S v2](pcb/LICENSE) |
| [`site/`](site/) | Source Hugo de [vigie.tadorne.org](https://vigie.tadorne.org/) | contenu [CC BY-SA 4.0](site/LICENSE-CONTENT), gabarits [MIT](site/LICENSE) |

Le contenu de `pcb/` (tag `v1.0`) est identique à l'archive publiée, signée et
horodatée :
[`vigie-1.0-kicad.zip`](https://vigie.tadorne.org/files/vigie-1.0/vigie-1.0-kicad.zip),
[signature](https://vigie.tadorne.org/files/vigie-1.0/vigie-1.0-kicad.zip.asc),
[horodatage FreeTSA](https://vigie.tadorne.org/files/vigie-1.0/vigie-1.0-kicad.zip.tsr).

## Authenticité

Les commits et les fichiers publiés sont signés par la clé Tadorne
(`5420 08E3 53EC A628 B4D6  ACAD 8C03 243A FF69 5BF7`) ; chaque commit est
horodaté par FreeTSA (RFC 3161), jeton rangé dans les notes Git
`refs/notes/freetsa`. Vérification : <https://tadorne.org/verify/>.

```sh
git fetch origin refs/notes/freetsa:refs/notes/freetsa
git notes --ref freetsa show HEAD
```
