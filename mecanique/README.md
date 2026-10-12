# Vigie — mécanique

## Semelle TPU pour coffret étanche IP67

Une seule pièce en TPU 95A qui tapisse le fond d'un coffret étanche du
commerce (moins les colonnes d'angle) et porte le shunt et la carte Vigie.
L'étanchéité est assurée par le coffret ; la semelle ne fait que le calage
mécanique et l'amortissement.

Coffret visé : [référence Amazon B0GGGHLTX5](https://www.amazon.com.be/dp/B0GGGHLTX5).
**Les cotes intérieures du fichier ne sont pas encore confirmées pour ce
modèle** : voir « Cotes à mesurer » ci-dessous.

- **Shunt 100 A / 75 mV** : berceau le long d'une grande paroi. Des lèvres
  droites de 3 × 2 mm débordent au-dessus des blocs ; on enfonce le shunt par
  le dessus en écartant les lèvres. Ouverture entre lèvres : 15,8 mm. La poche
  est 0,6 mm plus courte que le shunt, qui est pincé en longueur. Les deux
  têtes de boulon M8 (13,5 mm, entraxe 86 mm) entrent dans des logements, et
  rien ne touche la barre de manganin (elle chauffe, le TPU ramollit vers
  80 °C).
- **Carte Vigie 86 × 54 mm** : 4 colonnettes Ø9, vis M3 autotaraudeuses pour
  plastique, carte à 11 mm du fond, borniers J1/J2 vers le haut.
- **Fond plat**, 6 mm d'épaisseur. La semelle touche les quatre parois et les
  colonnes d'angle ; deux vis M4 avec rondelle large la plaquent au fond.
  Les picots moulés du fond du coffret sont à couper au ras.

### Durcissement (chocs et vibrations)

- Un **tampon** en TPU, collé sous le couvercle, appuie sur le berceau avec
  1,5 mm d'écrasement. Le shunt est serré entre le fond et le couvercle, sans
  jeu.
- Les lèvres font 3 mm. Les abouts du berceau sont échancrés jusqu'au-dessus
  des blocs pour que la cosse sorte à plat.
- Un **ancrage Rilsan** devant chaque about tient le câble de 16 mm², dont la
  masse ne fait plus levier sur le boulon M8.
- Les trous M4 reçoivent des **bagues de compression** Ø6 / Ø4,2 × 5,7 mm
  (entretoise inox lisse ou tube laiton), avec une vis inox A4, un écrou
  Nylstop et un trait de peinture témoin.

### Fichiers

```
semelle_shunt_TPU.scad                      source paramétrique (OpenSCAD)
STL_a_imprimer/semelle_shunt_TPU.stl        semelle
STL_a_imprimer/tampon_shunt_TPU.stl         tampon (déjà retourné pour l'impression)
STL_a_imprimer/modificateur_berceau_100pc.stl   modificateur de remplissage, voir plus bas
semelle_shunt_TPU95A.curaprofile            profil Cura de la semelle
tampon_shunt_TPU95A.curaprofile             profil Cura du tampon
```

Régénérer :

```sh
openscad -o STL_a_imprimer/semelle_shunt_TPU.stl -D 'part="semelle"' semelle_shunt_TPU.scad
openscad -o STL_a_imprimer/tampon_shunt_TPU.stl  -D 'part="tampon"'  semelle_shunt_TPU.scad
```

### Cotes à mesurer

Les marqueurs `>>> A MESURER` du source signalent les valeurs encore à relever
sur le coffret réel : fond intérieur, colonnes d'angle, hauteur des têtes de
boulon, profondeur sous le couvercle (`prof_int`, pour la hauteur du tampon),
entraxe des vis M4 (139,7 mm), position des vis Kelvin. **Ne pas imprimer sans
les avoir vérifiées.**

### Profils d'impression TPU 95A

Réglages TPU 95A communs aux projets Tadorne : 195 °C (205 à la 1re couche),
couches de 0,15 mm (1re couche 0,3), 2 parois de 0,45, rétraction de 1,5 mm à
20 mm/s avec coasting, combing `all`, accélération et jerk désactivés, bordure
de 6 mm, plateau à 55 °C (60 à la 1re couche). À importer dans Cura
(*Préférences → Profils → Importer*), machine Creality, qualité `standard`.

| Fichier | Remplissage |
|---|---|
| `semelle_shunt_TPU95A.curaprofile` | **25 % gyroïde** : la dalle sert d'amortisseur |
| `tampon_shunt_TPU95A.curaprofile` | 100 % en lignes : raide, pour tenir la précontrainte |

**Berceau imprimé plein.** Un profil Cura ne peut pas contenir de remplissage
local ; il faut charger `modificateur_berceau_100pc.stl` avec la semelle :

1. Les deux fichiers ont exactement le même encombrement (4 micro-cubes dans
   les coins vides du modificateur). Décocher *Préférences → Veiller à ce que
   les modèles restent séparés*, puis, dans l'outil Déplacer, mettre
   **X = 0 et Y = 0** pour la semelle comme pour le modificateur.
2. Sélectionner le modificateur, puis *Réglages par modèle → Modifier les
   réglages des chevauchements*.
3. Régler le remplissage à 100 %.

Le berceau, les lèvres et les ancrages sortent pleins, la dalle reste en
gyroïde.

**Orientation** : semelle posée à plat, le dessous contre le plateau ; tampon
tel qu'exporté, l'âme contre le plateau.
