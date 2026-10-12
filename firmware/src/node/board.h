// Brochage — identique sur la carte Vigie 1.0 (pcb/README.md §3) et sur une
// devkit ESP32 38 broches : seules la présence de l'INA226 et le choix du
// capteur par défaut changent.
#pragma once
#include <driver/gpio.h>

namespace board {

constexpr int kSda = 21;
constexpr int kScl = 22;
constexpr gpio_num_t kInaAlert = GPIO_NUM_4;  // RTC : réveil ext0, actif bas (R13 10 k)
constexpr int kLed = 2;                       // LED2 bleue « statut »
constexpr gpio_num_t kButton = GPIO_NUM_0;    // BOOT : réveil ext1, ouvre l'administration
constexpr uint8_t kInaAddr = 0x40;

// Réservés au futur RS485 du MPPT, sortis sur J7.
constexpr int kRs485Rx = 16;
constexpr int kRs485Tx = 17;
constexpr int kRs485De = 18;

#if defined(VIGIE_BOARD_V1)
constexpr const char *kName = "Vigie 1.0";
constexpr uint8_t kDefaultSensor = 0;  // auto : INA226, simulateur s'il ne répond pas
#else
constexpr const char *kName = "ESP32 devkit";
constexpr uint8_t kDefaultSensor = 2;  // simulateur
#endif

}  // namespace board
