# Firmware Vigie — nœud énergie

Firmware de la carte [Vigie 1.0](../pcb/README.md) (ESP32-WROOM-32E + INA226),
utilisable dès maintenant sur une **devkit ESP32 nue** : un simulateur de
batterie remplace l'INA226 absent, et chaque mesure simulée porte le drapeau
`SIM` jusque dans le journal et sur les ondes.

```sh
pio run -e devkit -t upload     # devkit : simulateur par défaut
pio run -e vigie  -t upload     # carte Vigie 1.0 : INA226, repli simulateur s'il ne répond pas
pio run -e gateway -t upload    # passerelle de banc ESP-NOW, sur un second ESP32
pio device monitor              # console série, 115200 bauds (help)
```

PlatformIO, plateforme pioarduino figée (Arduino-ESP32 3.3.11 / ESP-IDF 5.5).
Aucune bibliothèque externe.

## 1. Fonctionnement

```
          ┌───────────── démarrage à froid, ou appui sur BOOT ─────────────┐
          ▼                                                                │
  [ÉVEILLÉ] point d'accès « Vigie-XXXX » + page d'administration           │
          │  mesures, journal et push continuent au même rythme            │
          │  l'annonce BLE tourne en continu                               │
          │  fermeture après ap_window_s (5 min), repoussée par l'activité │
          ▼                                                                │
  [CYCLE DE SOMMEIL] ──► réveil timer ──► mesure ──► agrégat en RTC RAM ───┤
          ▲                   │            └─► enregistrement flash (5 min)│
          │                   └─► push (15 min) : ESP-NOW puis BLE ────────┤
          └────────────────────── sommeil profond ◄────────────────────────┘
               réveil anticipé : alerte INA226 (GPIO 4) ou bouton BOOT (GPIO 0)
```

| Cadence (défaut) | Paramètre | Coût mesuré sur devkit |
|---|---|---|
| Mesure toutes les **5 min** | `sample_s` | 110 ms d'éveil (+ ~250 ms de ROM et chargeur d'amorçage) |
| Annonce BLE brève à chaque mesure | `ble_every`, `ble_short_ms` | ~0,4 s d'éveil de plus, **estimé** — voir §4 |
| Enregistrement toutes les **5 min** | `log_s` | agrégat moyenne / min / max des mesures de la période |
| Push toutes les **15 min** | `push_s` | 1,8 s d'éveil, dont 1,1 s d'annonce BLE (`ble_ms`) |
| Hivernage : push toutes les **6 h** | `push_winter_s` | `mode` = 3 |

Estimation, **à mesurer** à la mise en service : ~0,05 mA pour les
mesures, ~0,05 mA pour les annonces BLE brèves, ~0,15 mA pour les push
(~0,01 mA en hivernage), **0,33 mA pour l'INA226 en continu**, 0,13 mA pour la
LED verte d'alimentation, ~0,05 mA de repos, soit **~0,75 mA** au port et
**~0,6 mA** en hivernage — dans l'enveloppe visée (0,8 à 1,3 mA).
L'INA226 et la LED verte pèsent désormais les deux tiers : voir §10.

Une mesure toutes les 5 min suffit à l'hivernage, où le courant varie peu.
Le comptage d'Ah, lui, n'échantillonne alors que 9,6 s sur 300 : une charge
brève (pompe de cale, démarreur) peut tomber entre deux mesures. En
navigation, repasser `sample_s` à 60 s.

## 2. Mesure et SOC

- **INA226** : moyenne de 1024 conversions (shunt 8,244 ms, bus 1,1 ms), soit
  une fenêtre glissante de **9,6 s** qui continue de moyenner pendant que
  l'ESP32 dort. Courant calculé depuis la tension de shunt brute : gain, zéro et
  inversion de signe (`i_gain`, `i_offset_mA`, `i_invert`) se règlent sans
  toucher au registre de calibration. Mode déclenché possible (`ina_cont` = 0) :
  ~2 µA au repos au lieu de 330 µA, au prix d'un comptage d'Ah échantillonné.
- **SOC** par comptage coulométrique (l'INA226 n'a pas d'accumulateur,
  voir le calcul du SOC), trapèzes entre deux mesures, rendement de charge 0,90, Peukert
  1,25 en décharge au-delà de C/20.
- **Resynchronisation à 100 %** (FR-05) : U ≥ `v_full` et 0 ≤ I < `i_tail`
  pendant `full_hold_s`.
- **Recalage au repos** (ajout) : après `ocv_rest_s` (4 h) à |I| faible, le SOC
  est rapproché de moitié de la table de tension à vide.
- Après une coupure d'alimentation, le SOC repart du **dernier enregistrement du
  journal** ; les cumuls d'Ah vivent en NVS (écrits toutes les heures).

## 3. Journal

Partition brute `vlog` de 896 Ko (`partitions_vigie.csv`), sans système de
fichiers : des enregistrements de **32 octets** avec CRC (`src/common/record.h`).

| Anneau | Capacité | À la cadence par défaut |
|---|---|---|
| Échantillons et événements | 27 520 | **~95 jours** à 5 min (19 jours à 1 min) |
| Résumés journaliers (Ah entrés/sortis, V et SOC min/max, T) | 896 | **~2,5 ans** |

Événements journalisés : démarrage (et sa cause), recalage d'horloge, recalage
de SOC, alertes, changements de configuration, entrée/sortie de coupure. La
numérotation `seq` est monotone à vie, effacement compris : un récepteur ne
revoit jamais deux fois le même numéro.

**Horloge.** La carte n'a pas d'horloge sauvegardée. Avant tout recalage,
`ts` compte depuis la mise sous tension (drapeau `time_valid` = 0). L'heure
arrive par la page d'administration (le téléphone la donne automatiquement à
l'ouverture), par la passerelle ESP-NOW, ou par la console (`time <epoch>`).
L'enregistrement `time_sync` permet de dater après coup ce qui précède
(`tools/vigie_log.py` le fait).

## 4. Push radio

### ESP-NOW — TsubameBus

En-tête de 12 octets (petit-boutiste, magic `42 54`) + corps CBOR, canal 1.

| Message | Quand | Contenu |
|---|---|---|
| `HELLO` 0x01 | premier push après un démarrage à froid | id, type, fw, capacités, cadences, dernier `seq` |
| `ENERGY` 0x10 | chaque push | champs du message ENERGY de TsubameBus (`ts v i p soc q t_bat mode flags`) + ajouts : `ah_in ah_out ahd_in ahd_out vmin_d vmax_d rec` |
| `ALERT` 0x20 | dès qu'une alerte se lève (push anticipé) | `code sev value limit` |
| `LOG_BATCH` 0x18 | après ENERGY, si un récepteur a déjà répondu | 6 enregistrements bruts par trame, `en_backlog` trames max par push |

**Rattrapage de l'historique.** Le récepteur accuse (`ACK` 0x21, champ `rec`)
le dernier enregistrement reçu ; le nœud ne renvoie que ce qui n'a pas été
accusé et s'arrête au premier accusé manquant. Un récepteur absent une semaine
récupère donc la semaine entière à son retour. Les accusés et `WELCOME`
peuvent porter `utc` : le nœud recale son horloge dessus.

Drapeaux `flags` ajoutés (bits 8-12) : `SIMULATED`, `TIME_UNSYNC`,
`SENSOR_ERR`, `SURVIVAL`, `CUTOFF`. Chiffrement : PMK + LMK en unicast
(`en_peer`, `en_pmk`, `en_lmk`) ; la diffusion reste en clair (limite
d'ESP-NOW). Mode longue portée ESP32 `en_lr`.

### BLE — BTHome v2

Annonce **[BTHome v2](https://bthome.io)** : lue telle quelle par Home
Assistant, l'application nRF Connect ou `tools/vigie_ble.py`. Objets : batterie
(= SOC), température, tension, courant et puissance signés. Chiffrement AES-CCM
optionnel (`ble_key`). La réponse au scan porte le nom et un bloc constructeur
(identifiant 0xFFFF) : `'V'`, version 1, MAC Bluetooth (que iOS et macOS
masquent, alors que le nonce BTHome en a besoin), drapeaux d'état et alertes
actives — en clair, même annonce chiffrée. L'app iPhone
s'en sert.

**Hors push, la radio est coupée** : en sommeil profond, rien n'est émis.
L'annonce dure `ble_ms` (1,1 s) à chaque push, et tourne en continu pendant la
fenêtre d'administration. Pour qu'un téléphone reçoive une valeur à chaque
mesure (toutes les 5 min) plutôt qu'à chaque push (15 min), **`ble_every`**
(actif par défaut) ajoute une annonce brève de `ble_short_ms` (300 ms,
~3 annonces) à chaque réveil de mesure, sans Wi-Fi, coupée en survie et en
coupure. Coût estimé, non mesuré : ~0,4 s d'éveil par mesure, soit ≈ 0,05 mA
en moyenne à 5 min (≈ 0,2 mA si `sample_s` repasse à 60 s). Le Bluetooth ne peut
pas rester actif pendant le sommeil : sur ESP32, il faudrait un quartz 32 kHz,
absent de Vigie 1.0.

Le contrôleur est piloté en HCI brut (VHCI), sans pile
Bluedroid : démarrage rapide à chaque réveil et empreinte mémoire réduite.

## 5. Protection de la batterie

| Tension | État | Effet |
|---|---|---|
| ≥ 12,2 V | nominal | |
| < `v_throttle` 12,2 V | réduit | push ÷ 2 |
| < `v_survival` 11,9 V | survie | push toutes les 6 h, pas de rattrapage |
| < `v_cutoff` 11,5 V | **coupure** | alerte finale, puis silence radio ; mesure horaire ; réveil par l'INA226 dès que U > `v_restart` (12,6 V) |

Une dégradation exige **deux évaluations consécutives** : un démarrage moteur
ne coupe pas le nœud. Aucune commande radio ne peut désactiver la coupure.

Alertes : `soc_warn`, `soc_alarm` (répétée toutes les 6 h), `volt_low`,
`volt_critical`, `temp_high`, `overcurrent` (aussi par l'ALERT matérielle de
l'INA226 pendant le sommeil), `no_charge_30d`, `boot_unexpected`,
`sensor_lost`, `volt_cutoff`.

## 6. Administration

Au démarrage à froid, ou sur appui de **BOOT** pendant le sommeil : point
d'accès **`Vigie-XXXX`** (WPA2, mot de passe par défaut **`vigie-admin`**, à
changer), portail captif, page sur `http://192.168.4.1/`.

- **Tableau de bord** : mesures, SOC, cumuls, état du journal et de la radio ;
- **Historique** : graphique tension / courant / SOC de 6 h à 30 j, exports CSV
  (échantillons, résumés journaliers) et binaire ;
- **Réglages** : tous les paramètres, générés depuis la table de
  `src/node/config.cpp`, avec bornes vérifiées côté nœud ;
- **Maintenance** : heure, recalage du SOC, push immédiat, scénario du
  simulateur, **mise à jour OTA**, effacement du journal.

La fenêtre dure `ap_window_s` (5 min) ; chaque requête la repousse de 2 min —
la page ouverte interroge le nœud toutes les 5 s —, dans la limite d'une heure
pour qu'un téléphone oublié ne vide pas la batterie.

La **console série** offre les mêmes fonctions (`help`), plus `dump` pour
extraire le journal par l'USB.

## 7. Outils

| Outil | Rôle |
|---|---|
| `tools/vigie_log.py` | décode un `.bin` ou lit le journal par l'USB (`--serial PORT`, `--days`), recale les dates, exporte en CSV |
| `tools/vigie_ble.py` | écoute les annonces BTHome, déchiffre avec `--key` |
| `src/gateway/` | passerelle de banc : imprime chaque trame en JSON, accuse, donne l'heure (`T<epoch>` sur son port série) |

Les scripts Python se lancent avec `uv run` (dépendances déclarées en tête de fichier).

## 8. Simulateur

Batterie plomb de `cap_ah`, panneau 100 W à plat (heure, saison, nébulosité
aléatoire), acceptation de charge décroissante près de 100 % (absorption puis
floating), charges du bord. Scénarios (`sim_scn`) : **0** port, **1**
navigation (instruments, pilote, moteur matin et soir, feux), **2** hivernage,
**3** décharge 4 A, **4** quai (chargeur 15 A). L'état vit en RTC RAM : la
simulation rattrape le temps passé en sommeil.

## 9. Vérifié sur la devkit

| Essai | Résultat |
|---|---|
| Compilation `devkit`, `vigie`, `gateway` | sans avertissement ; nœud 1,25 Mo (79 % d'un emplacement OTA) |
| Cycle de sommeil 10 s / 20 s / 30 s | réveils à l'heure, 110 ms d'éveil par mesure |
| Journal, reprise après reset, décodage `vigie_log.py` | SOC restauré, dates recalées a posteriori |
| Resynchro 100 % (scénario quai) | 14,40 V, 1,18 A, 90 → 100 % après le maintien |
| Coupure (décharge) puis reprise (quai) | alerte finale, silence radio, reprise à 13,2 V et push immédiat |
| Émission ESP-NOW | trames acquittées par la couche MAC (diffusion) |
| Contrôleur BLE | toutes les commandes HCI acceptées, annonce active |
| Page d'administration | rendu et formulaires vérifiés dans un navigateur sur une maquette rejouant les réponses du nœud |

**Pas encore vérifié** : la réception réelle des trames ESP-NOW (il faut un
second ESP32 avec `env:gateway`), la réception BLE (le Mac de développement
n'avait pas l'autorisation Bluetooth), la page servie par le nœud lui-même, et
tout ce qui touche à l'INA226 réel (ALERT, calibration, signe) — à faire à la
réception des cartes.

## 10. Pistes

- **Mesurer le courant de repos** avant tout arbitrage. Si l'INA226 en
  continu pèse trop, passer `ina_cont` à 0 et `sample_s` à 60 s ; la LED verte
  (R6) peut être dépeuplée.
- **Heure** : sans quartz 32 kHz, le sommeil est cadencé par l'oscillateur RC
  interne, dont la dérive est à mesurer (de l'ordre du pour cent selon la
  température, soit potentiellement des minutes par jour). La passerelle (module B)
  doit donner l'heure à chaque accusé ; un quartz 32 kHz sur une prochaine
  révision de la carte la ferait tenir seule.
- **Commandes descendantes** (TsubameBus) : la structure de réception existe
  (`espnow::poll`) ; `soc_reset`, `set_config`, `uplink_now` s'y ajoutent sans
  toucher au reste.
- **Mode STA + NTP** pendant la fenêtre d'administration, pour les ports
  équipés d'un Wi-Fi.
