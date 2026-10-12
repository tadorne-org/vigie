// Enregistrements du journal flash — 32 octets, petit-boutiste, CRC16 final.
//
// Le même octet-à-octet voyage dans les messages LOG_BATCH et sort par
// /log.bin : tout outil qui lit l'un lit l'autre (voir tools/vigie_log.py).
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace vlog {

constexpr int kRecordSize = 32;
constexpr int16_t kNoTemp = INT16_MIN;

enum RecType : uint8_t {
  REC_SAMPLE = 1,     // agrégat d'une période de journalisation
  REC_BOOT = 2,       // démarrage à froid : dt_s = cause de reset, i_avg = n° de démarrage
  REC_TIME_SYNC = 3,  // recalage d'horloge : i_avg = décalage appliqué (s)
  REC_SOC_SET = 4,    // SOC recalé : soc = nouvelle valeur, dt_s = cause (SocCause)
  REC_ALERT = 5,      // alerte : dt_s = code (AlertCode), mesures = instantané
  REC_CONFIG = 6,     // configuration modifiée
  REC_CUTOFF = 7,     // entrée / sortie de coupure sous-tension (dt_s = 1 / 0)
};

enum RecFlags : uint8_t {
  RF_SIM = 1 << 0,
  RF_TIME_VALID = 1 << 1,
  RF_CHARGING = 1 << 2,
  RF_UNDERVOLT = 1 << 3,
  RF_SOC_SYNC = 1 << 4,
  RF_SENSOR_ERR = 1 << 5,
  RF_SURVIVAL = 1 << 6,
  RF_CUTOFF = 1 << 7,
};

enum SocCause : uint16_t { SOC_FULL = 1, SOC_OCV = 2, SOC_MANUAL = 3, SOC_RESTORE = 4 };

enum AlertCode : uint16_t {
  AL_SOC_WARN = 1,
  AL_SOC_ALARM,
  AL_VOLT_LOW,
  AL_VOLT_CRITICAL,
  AL_TEMP_HIGH,
  AL_OVERCURRENT,
  AL_NO_CHARGE_30D,
  AL_BOOT_UNEXPECTED,
  AL_SENSOR_LOST,
  AL_CUTOFF,
};

inline const char *alertName(uint16_t c) {
  switch (c) {
    case AL_SOC_WARN: return "soc_warn";
    case AL_SOC_ALARM: return "soc_alarm";
    case AL_VOLT_LOW: return "volt_low";
    case AL_VOLT_CRITICAL: return "volt_critical";
    case AL_TEMP_HIGH: return "temp_high";
    case AL_OVERCURRENT: return "overcurrent";
    case AL_NO_CHARGE_30D: return "no_charge_30d";
    case AL_BOOT_UNEXPECTED: return "boot_unexpected";
    case AL_SENSOR_LOST: return "sensor_lost";
    case AL_CUTOFF: return "volt_cutoff";
  }
  return "unknown";
}

// Échantillon agrégé (et événements, qui réutilisent les champs).
struct __attribute__((packed)) Record {
  uint32_t seq;       // numéro monotone, 0xFFFFFFFF = emplacement effacé
  uint32_t ts;        // epoch UTC si RF_TIME_VALID, sinon s depuis la mise sous tension
  int32_t i_avg_mA;   // courant moyen, + = charge
  uint16_t v_avg_mV;
  uint16_t v_min_mV;
  uint16_t v_max_mV;
  int16_t i_min_cA;   // centi-ampères (±327 A)
  int16_t i_max_cA;
  uint16_t soc_cp;    // SOC en centièmes de %
  int16_t t_cC;       // température en centièmes de °C, kNoTemp = absente
  uint16_t dt_s;      // durée couverte ; pour un événement : argument
  uint8_t type_mode;  // bits 0-3 : RecType, bits 4-5 : mode
  uint8_t flags;      // RecFlags
  uint16_t crc;       // CRC16-CCITT des 30 premiers octets
};
static_assert(sizeof(Record) == kRecordSize, "Record : 32 octets");

// Résumé journalier.
struct __attribute__((packed)) DayRecord {
  uint32_t seq;
  uint32_t ts;        // début de la journée
  uint32_t mah_in;    // charge entrée dans la batterie
  uint32_t mah_out;   // charge sortie
  uint16_t v_min_mV;
  uint16_t v_max_mV;
  uint16_t soc_min_cp;
  uint16_t soc_max_cp;
  int16_t t_min_cC;
  int16_t t_max_cC;
  uint8_t flags;      // RF_SIM, RF_TIME_VALID
  uint8_t hours;      // heures couvertes (24 sauf journée partielle)
  uint16_t crc;
};
static_assert(sizeof(DayRecord) == kRecordSize, "DayRecord : 32 octets");

inline uint16_t crc16(const void *data, size_t n) {
  const uint8_t *p = (const uint8_t *)data;
  uint16_t crc = 0xFFFF;
  while (n--) {
    crc ^= (uint16_t)(*p++) << 8;
    for (int i = 0; i < 8; i++) crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
  }
  return crc;
}

template <typename T>
inline void seal(T &r) { r.crc = crc16(&r, kRecordSize - 2); }
template <typename T>
inline bool valid(const T &r) { return r.seq != 0xFFFFFFFF && r.crc == crc16(&r, kRecordSize - 2); }

}  // namespace vlog
