// Source de mesure : INA226 réel, ou modèle simulé d'une batterie plomb
// 73 Ah avec panneau 100 W, charges du bord, moteur et chargeur de quai.
#pragma once
#include <stdint.h>

struct Reading {
  bool ok = false;
  float v = 0;   // V, tension batterie
  float i = 0;   // A, + = charge
  float t = 0;   // °C
  bool t_ok = false;
  bool sim = false;
};

// Choisit la source d'après cfg.sensor ; en « auto », l'INA226 s'il répond.
void sensorBegin(bool cold);
bool sensorRead(Reading &r);
const char *sensorName();
bool sensorIsSim();
// Prépare le sommeil : arme l'alerte INA226 si utile, coupe l'INA en mode déclenché.
// alert_over_v > 0 : réveil quand la tension dépasse ce seuil (sortie de coupure).
// Rend vrai si la broche ALERT peut servir de source de réveil.
bool sensorSleep(float alert_over_v, float alert_over_a);

// Simulateur, exposé pour la page d'administration.
const char *simScenarioName(uint8_t s);
void simReset(float soc);
