// Estimation du SOC, en logiciel : l'INA226 n'a pas
// d'accumulateur de charge.
#pragma once
#include <stdint.h>

#include "sensor.h"

// Tension à vide d'une batterie plomb 12 V au repos, 25 °C.
float ocvFromSoc(float soc);
float socFromOcv(float v);

enum class SocEvent : uint8_t { NONE, FULL, OCV, INIT };

// Intègre une mesure. `dt_s` : temps écoulé depuis la mesure précédente.
// Rend l'éventuel recalage effectué, pour journalisation.
SocEvent batteryUpdate(const Reading &r, uint32_t now, float dt_s, float &soc_before);
void batterySetSoc(float soc);
