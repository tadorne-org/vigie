// Interface de l'application, pour la page d'administration et la console.
#pragma once
#include <Arduino.h>

#include "sensor.h"

namespace app {
String statusJson();
const Reading &last();
void sampleNow();
void pushNow();
void setSoc(float soc);
bool eraseLogs();
void sleepNow();         // ferme l'administration et repart en cycle de sommeil
void configChanged();    // après modification de cfg (déjà enregistrée)
void setTime(int64_t epoch, const char *source);
void simReset(float soc);
}  // namespace app

namespace admin {
void start();
void stop();
void loop();
bool active();
void extend(uint32_t seconds);
void close();       // fermeture à la prochaine itération de loop()
uint32_t remaining();  // secondes avant fermeture
}  // namespace admin

namespace cli {
void poll();
}
