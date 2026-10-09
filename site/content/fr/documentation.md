---
title: "Documentation technique"
translationKey: documentation
url: /fr/documentation/
description: "Architecture, carte, mesure, micrologiciel et réseau de Vigie 1.0 — travail en cours."
params:
  eyebrow: "Vigie 1.0"
---

{{< callout title="Travail en cours" >}}
Cette page est la documentation de **Vigie 1.0**, encore en cours d'élaboration.

**[Voir le journal](/fr/journal/)** pour l'état le plus récent.
{{< /callout >}}

Vigie est une carte unique qui mesure l'énergie d'un voilier ou d'un pack
batterie 12 V, veille toute l'année et rend compte. Elle remplace l'empilage
« devkit ESP32 + module INA226 + convertisseur LM2596 » par une carte de
86 × 54 mm à quatre couches, et se place **en amont du coupe-circuit** — le seul
module qui reste vivant l'hiver : ses convertisseurs se contentent de 22 µA par
étage, et l'ensemble reste sous le milliampère.

<figure class="doc">
  <img src="/img/doc/board-top.webp" alt="Rendu de la carte Vigie 1.0 côté composants.">
  <figcaption><b>Vigie 1.0</b>, côté composants : borniers 12 V et shunt à gauche, ESP32-WROOM-32E à droite, convertisseurs au centre, logo « Vigie 1.0 — by Tadorne » en sérigraphie.</figcaption>
</figure>

## Vue d'ensemble

Un seul point de vérité : tout le courant du bord passe par un **shunt de
100 A / 75 mV** monté dans le bus négatif, et la carte ne fait que lire la
tension à ses bornes, en quatre fils. Aucun courant de charge ne la traverse.

<figure class="doc">
  <img loading="lazy" decoding="async" src="/img/doc/vigie-blocs-fr.svg" alt="Schéma-bloc de Vigie 1.0 : alimentation, mesure, programmation et extensions autour de l'ESP32.">
  <figcaption>Schéma-bloc de la carte : chaîne d'alimentation 12 V → 5 V → 3,3 V, mesure INA226 en Kelvin, programmation USB-C et extensions, autour de l'ESP32-WROOM-32E.</figcaption>
</figure>

| Caractéristique | Valeur |
|---|---|
| Dimensions | 86 × 54 mm, coins R2, quatre trous M3 |
| Couches | 4 : signal / plan de masse / plan +3,3 V / signal |
| Microcontrôleur | ESP32-WROOM-32E, antenne PCB au flanc droit, zone sans cuivre sur les quatre couches |
| Mesure | INA226AIDGSR, shunt externe 100 A / 75 mV, liaison Kelvin quatre fils |
| Alimentation | 12 V → 5 V → 3,3 V, 2 A par étage, courant de repos 22 µA par étage |
| Protections | polyfuse, P-MOSFET anti-inversion, TVS SMAJ16A, fusible 100 mA sur VBUS |
| Programmation | CH340C + USB-C, réinitialisation automatique DTR/RTS |
| Extension | deux barrettes 1×8 : 12 GPIO, +3,3 V, +5 V, I2C, UART, RS485 réservé |
| Empreintes | 69 empreintes et 4 trous, 54 nets |

## La carte

### L'alimentation

Deux convertisseurs à découpage de la même famille se partagent la chaîne :
**AP63205WU** pour le 12 V → 5 V et **AP63203WU** pour le 5 V → 3,3 V, 2 A
chacun à 1,1 MHz. Le point dur d'un nœud alimenté toute l'année est leur
courant de repos : **22 µA par étage**, là où un LM2596 en consomme 5 à 10 mA
en permanence, soit environ 3,5 Ah par mois prélevés pour rien.

L'entrée est protégée par une polyfuse, un P-MOSFET anti-inversion (45 mΩ, avec
sa zener de grille — obligatoire, la batterie monte à 14,4 V en charge) et une
TVS SMAJ16A. Sur la paillasse, le cavalier **J5** alimente la carte par l'USB ;
il est ouvert par défaut pour que le 5 V ne remonte pas jusqu'au bornier
batterie. Le **CH340C** est alimenté par un LDO pris sur le VBUS de l'USB seul :
débranché, il ne consomme rien.

### La mesure

L'INA226 travaille sur un shunt de **0,75 mΩ** placé dans le bus négatif :
75 mV de mode commun au lieu de 12 V, et `VBUS` lit directement la tension
batterie. Sa plage est figée à **±81,92 mV** — le bit `ADCRANGE` n'existe pas
sur ce composant, contrairement à l'INA228 — ce qui donne, avec le pas natif de
2,5 µV, une résolution de **3,33 mA**. Cent ampères occupent 91,6 % de
l'échelle.

Le gain, le zéro et le signe du courant se règlent en logiciel (`i_gain`,
`i_offset_mA`, `i_invert`) : un J2 câblé à l'envers se corrige sans redémonter
la carte. Le comptage d'ampères-heures est logiciel lui aussi, l'INA226 n'ayant
pas d'accumulateur de charge. Enfin, `ALERT` arrive sur **GPIO 4**, une broche
RTC : elle peut réveiller l'ESP32 d'un sommeil profond.

<figure class="doc">
  <img loading="lazy" decoding="async" src="/img/doc/shunt-busbar.webp" alt="Montage du nœud au bus négatif : shunt, paire Kelvin et busbar.">
  <figcaption>Le montage au bus négatif, figure d'étude : la paire Kelvin se
  prend sur les <b>petites vis de mesure</b> du shunt, jamais sous les boulons de
  puissance, et le retour de la carte rejoint le bus négatif au plus près du
  shunt. Le principe est celui de Vigie 1.0, qui ne transporte aucun courant de
  charge.</figcaption>
</figure>

La précaution n'est pas théorique : à 100 A, 20 cm de câble 16 mm² valent déjà
23 mV, soit près d'un tiers de l'échelle de mesure.

### Le routage et l'implantation

Les deux couches internes sont des **plans pleins, sans aucune piste** : In1
pour la masse, In2 pour le +3,3 V. Chaque pastille CMS de masse et de +3,3 V
descend au plan par son propre via, posé et verrouillé avant le routage. Le
routage des signaux tient sur les deux couches externes.

<figure class="doc">
  <img loading="lazy" decoding="async" src="/img/doc/cu-front.webp" alt="Routage de la couche avant de Vigie 1.0.">
  <figcaption>Routage de la couche avant (F.Cu) : les signaux tiennent sur les
  deux couches externes, les deux couches internes restant des plans pleins.</figcaption>
</figure>

<figure class="doc">
  <img loading="lazy" decoding="async" src="/img/doc/place.webp" alt="Implantation et sérigraphie de Vigie 1.0.">
  <figcaption>Implantation et sérigraphie : chaque repère est placé là où il ne
  recouvre ni pastille ni via.</figcaption>
</figure>

### Connecteurs

| Réf. | Rôle | Brochage |
|---|---|---|
| J1 | Entrée 12 V (bornier 5,08 mm) | 1 = +12 V batterie (fusible 2 A externe) · 2 = GND, côté charge du bus négatif |
| J2 | Shunt (bornier 3,5 mm) | 1 = IN− · 2 = IN+ — sens à valider à la mise en service |
| J3 | UART / ISP | 3V3 · GND · TX · RX · IO0 · EN |
| J4 | I2C | 3V3 · GND · SDA · SCL |
| J5 | Cavalier USB PWR | fermer pour alimenter par l'USB, hors batterie uniquement |
| J6 | USB-C | programmation |
| J7 | Extension 1 | 3V3 · IO16 · IO17 · IO5 · IO18 · IO19 · IO23 · GND |
| J8 | Extension 2 | 5V · GND · IO13 · IO14 · IO26 · IO25 · IO33 · IO32 |

L'I2C est sur SDA 21 / SCL 22, le voyant de statut sur GPIO 2, le bouton BOOT
sur GPIO 0, la console sur TX 1 / RX 3. Les GPIO 16, 17 et 18 sont groupés sur
J7 pour le futur RS485 du MPPT. Les GPIO de strapping 12 et 15, comme les
GPIO 34 à 39, ne sont pas sortis.

## Le micrologiciel

Le micrologiciel tourne sur PlatformIO, plateforme pioarduino figée
(Arduino-ESP32 3.3.11 / ESP-IDF 5.5), sans aucune bibliothèque externe. Il
fonctionne dès maintenant sur une devkit ESP32 nue : un simulateur de batterie
remplace l'INA226 absent, et chaque mesure simulée porte le drapeau `SIM`
jusque dans le journal et sur les ondes.

Le cycle est un sommeil profond entrecoupé de réveils courts :

| Tâche | Période par défaut | Coût |
|---|---|---|
| Mesure | 5 min | 110 ms d'éveil |
| Enregistrement | 5 min | moyenne, minimum et maximum de la période |
| Annonce BLE brève | à chaque mesure | environ 0,4 s d'éveil, estimé |
| Push radio | 15 min | 1,8 s d'éveil, dont 1,1 s d'annonce BLE |
| Hivernage | push toutes les 6 h | mode survie |

La mesure s'appuie sur une moyenne de 1024 conversions de l'INA226, soit une
fenêtre glissante de **9,6 s** qui continue de moyenner pendant que l'ESP32
dort. L'état de charge est un comptage coulométrique : trapèzes entre deux
mesures, rendement de charge 0,90, exposant de Peukert 1,25 en décharge
au-delà de C/20. Il se resynchronise à 100 % quand la tension tient au-dessus
du seuil avec un courant de queue faible, et se recale sur la table de tension
à vide après quatre heures au repos.

Le journal vit dans une partition brute de 896 Ko, sans système de fichiers :
des enregistrements de 32 octets protégés par un CRC. À la cadence par défaut,
l'anneau garde environ **95 jours** d'échantillons et **2,5 ans** de résumés
journaliers. La carte n'a pas d'horloge sauvegardée : l'heure arrive par la
page d'administration, par la passerelle ou par la console, et un
enregistrement `time_sync` permet de dater après coup ce qui précède.

La protection de la batterie est un escalier, et une dégradation exige deux
évaluations consécutives — un démarrage moteur ne coupe pas le nœud :

| Tension | État | Effet |
|---|---|---|
| 12,2 V et plus | nominal | |
| moins de 12,2 V | réduit | push divisé par deux |
| moins de 11,9 V | survie | push toutes les 6 h, sans rattrapage |
| moins de 11,5 V | coupure | alerte finale, puis silence radio ; réveil par l'INA226 dès que la tension repasse au-dessus de 12,6 V |

L'administration se fait par un point d'accès `Vigie-XXXX` (WPA2, portail
captif), ouvert au démarrage à froid ou par appui sur BOOT : tableau de bord,
historique de 6 h à 30 jours avec exports CSV, réglages, recalage du SOC, mise
à jour OTA et effacement du journal. La console série offre les mêmes
fonctions.

<figure class="doc">
  <img loading="lazy" decoding="async" src="/img/v1-assembled.webp" alt="Carte Vigie 1.0 assemblée, alimentée par l'USB-C, sur le banc.">
  <figcaption>La première carte assemblée : 3,3 V présent, CH340C reconnu, micrologiciel lancé, INA226 qui répond sur l'I2C.</figcaption>
</figure>

<figure class="doc">
  <img loading="lazy" decoding="async" src="/img/v1-dashboard.webp" alt="Tableau de bord de Vigie 1.0 servi depuis le point d'accès de la carte.">
  <figcaption>La page d'administration, servie depuis le point d'accès de la carte. Sans shunt, les colonnes de courant affichent encore le fond d'échelle de l'INA226.</figcaption>
</figure>

## Le réseau et l'application

Le nœud pousse ses mesures de deux façons complémentaires. Sur le réseau local,
**TsubameBus** : des trames ESP-NOW chiffrées, en-tête de 12 octets et corps
CBOR, avec `HELLO`, `ENERGY`, `ALERT` et `LOG_BATCH`. Un récepteur accuse
réception du dernier enregistrement reçu, si bien qu'un module absent une
semaine récupère la semaine entière à son retour. Vers les téléphones, une
annonce **[BTHome v2](https://bthome.io)** en Bluetooth LE, lisible telle quelle
par Home Assistant ou nRF Connect, avec chiffrement AES-CCM optionnel.

L'implémentation de [Signal K](https://signalk.org/) est prévue.

| Diffusion de Vigie | Reçue par un iPhone | Contenu |
|---|---|---|
| Annonce BLE BTHome v2 | oui | état de charge, tension, courant, puissance, température |
| Réponse au scan (bloc constructeur) | oui | MAC Bluetooth, drapeaux d'état, alertes actives |
| Trames ESP-NOW | non | iOS ne donne accès à aucune trame Wi-Fi brute |

L'application iPhone **Tadorne** écoute passivement : aucune connexion, aucun
appairage, rien n'est émis. Elle garde l'historique de ce qu'elle a capté, app
ouverte ; le journal complet reste dans la flash du nœud.

<div class="photo-grid">
  <img loading="lazy" decoding="async" src="/img/doc/app-fr-1.webp" alt="Fiche d'un nœud dans l'application Tadorne : état de charge, mesures et graphique.">
  <img loading="lazy" decoding="async" src="/img/doc/app-fr-2.webp" alt="Liste des nœuds dans l'application Tadorne.">
  <img loading="lazy" decoding="async" src="/img/doc/app-fr-3.webp" alt="Historique du courant dans l'application Tadorne.">
</div>

<p class="meta">Application Tadorne — fiche d'un nœud, liste des nœuds, historique du courant.</p>

Cette révision de la carte n'embarque **ni LoRa ni LTE** : elle est le poste de
mesure, pas la passerelle Internet. L'arbitrage radio reste ouvert, et la
passerelle viendra s'ajouter au réseau sans modifier la carte.

## État et points ouverts

| Point | État |
|---|---|
| Courant de repos réel | à mesurer à la mise en service — c'est lui qui fixe l'autonomie hivernale |
| Signe du courant au shunt | à valider au premier câblage |
| Antenne en coffret fermé, en fond de cale | point ouvert ; la variante ESP32-WROOM-32UE, à antenne déportée u.FL, se monte sur la même empreinte |
| RS485 du MPPT EPEver | non implanté ; module MAX3485 externe sur J7 |
| Micrologiciel | à publier |
| Nomenclature et procédure d'étalonnage | à venir avec le micrologiciel |

Le banc a validé la compilation des trois environnements, le cycle de sommeil,
le journal et sa reprise après reset, la resynchronisation à 100 % et la
coupure basse tension. La réception réelle des trames ESP-NOW, la réception BLE
et tout ce qui touche à l'INA226 réel restent à vérifier sur la carte.

## Source : le journal

Chaque note du journal est la source de cette page, et le journal fait foi.

{{< log-sources >}}
