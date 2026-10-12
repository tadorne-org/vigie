// TsubameBus — bus ESP-NOW inter-modules.
//
// Trame = en-tête fixe de 12 octets + corps CBOR. Tous les champs de
// l'en-tête sont en petit-boutiste (ordre natif de l'ESP32) : le magic
// 0x5442 apparaît donc sur l'air sous la forme 42 54.
#pragma once
#include <stdint.h>

namespace tbus {

constexpr uint16_t kMagic = 0x5442;
constexpr uint8_t kSchema = 1;
constexpr int kMaxFrame = 250;  // limite ESP-NOW v1
constexpr int kHeaderLen = 12;
constexpr int kMaxPayload = kMaxFrame - kHeaderLen;

enum MsgType : uint8_t {
  HELLO = 0x01,
  WELCOME = 0x02,
  ENERGY = 0x10,
  NAV = 0x11,
  ENV = 0x12,
  ENGINE = 0x13,
  ATTITUDE = 0x14,
  WIND = 0x15,
  // Ajout de Vigie, additif : lot d'enregistrements bruts du
  // journal flash, pour le rattrapage après une absence du récepteur.
  LOG_BATCH = 0x18,
  ALERT = 0x20,
  ACK = 0x21,
  CMD = 0x30,
  CMD_RESULT = 0x31,
  PAIR = 0x40,
  SLEEP_HINT = 0x50,
};

struct __attribute__((packed)) Header {
  uint16_t magic;
  uint8_t schema;
  uint8_t msg_type;
  uint16_t src_id;
  uint16_t seq;
  uint32_t uptime_s;
};
static_assert(sizeof(Header) == kHeaderLen, "en-tête TsubameBus : 12 octets");

// Drapeaux du champ `flags` du message ENERGY (u16). Bits 0-4 : spécification TsubameBus.
enum EnergyFlags : uint16_t {
  F_CHARGING = 1 << 0,
  F_SOC_RESYNC = 1 << 1,
  F_UNDERVOLT = 1 << 2,
  F_MPPT_MUTE = 1 << 3,
  F_LTE_DOWN = 1 << 4,
  // Ajouts Vigie
  F_SIMULATED = 1 << 8,    // mesure issue du simulateur, pas d'un capteur
  F_TIME_UNSYNC = 1 << 9,  // horloge jamais recalée : `ts` = secondes depuis la mise sous tension
  F_SENSOR_ERR = 1 << 10,
  F_SURVIVAL = 1 << 11,    // mode survie (cadence réduite)
  F_CUTOFF = 1 << 12,      // coupure sous-tension (radio muette sauf alerte)
};

enum Severity : uint8_t { SEV_INFO = 0, SEV_WARN = 1, SEV_CRIT = 2 };

}  // namespace tbus
