// Push radio : ESP-NOW (TsubameBus) et annonce BLE BTHome v2.
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "sensor.h"

namespace espnow {
// `ap_active` : le Wi-Fi tourne déjà en point d'accès, on s'y greffe.
bool begin(bool ap_active);
void end();
bool ready();
// Envoie une trame (en-tête + corps CBOR). Rend vrai si la couche MAC l'a
// acquittée (unicast) ou émise (diffusion).
bool send(uint8_t type, const uint8_t *payload, size_t n);
// Traite les messages reçus (ACK, WELCOME) ; à appeler après les envois.
void poll(uint32_t wait_ms);

void sendHello();
void sendEnergy(const Reading &r);
void sendAlert(uint16_t code, uint8_t sev, float value, float limit);
// Rattrapage : envoie l'historique non acquitté. Rend le nombre de trames.
int sendBacklog();

extern uint32_t frames_ok, frames_fail;
// Drapeaux d'état tbus::EnergyFlags, partagés avec l'annonce BLE.
uint16_t energyFlags();
}  // namespace espnow

namespace bthome {
bool start();
void update(const Reading &r, float soc);
void stop();
bool running();
void initCounter();  // démarrage à froid : réserve une plage de compteurs en NVS
extern uint32_t hci_ok, hci_fail, adverts;
}  // namespace bthome
