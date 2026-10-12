// Configuration persistante (NVS).
//
// Une seule table (kFields, config.cpp) décrit chaque paramètre : clé NVS,
// type, bornes, valeur par défaut, libellé. Elle sert à la fois au chargement,
// à l'enregistrement, au formulaire de la page d'administration et à la
// commande `set` de la console série. Ajouter un paramètre = une ligne.
#pragma once
#include <Arduino.h>

struct Config {
  // Identité
  char name[24];
  uint16_t node_id;
  uint8_t mode;  // 0 NAV, 1 PORT, 2 MOUILLAGE, 3 HIVERNAGE
  // Cadences
  uint16_t sample_s;
  uint16_t log_s;
  uint32_t push_s;
  uint32_t push_winter_s;
  // Batterie
  float cap_ah;
  float peukert;
  float eta_chg;
  float v_full;
  float i_tail;
  uint16_t full_hold_s;
  uint32_t ocv_rest_s;
  // Seuils
  float soc_warn;
  float soc_alarm;
  float v_low;
  float v_crit;
  float t_high;
  float i_over;
  float v_throttle;
  float v_survival;
  float v_cutoff;
  float v_restart;
  // Capteur
  uint8_t sensor;  // 0 auto, 1 INA226, 2 simulateur
  float shunt_mohm;
  float i_gain;
  float i_offset_mA;
  bool i_invert;
  bool ina_cont;
  uint8_t sim_scn;
  // ESP-NOW
  bool en_espnow;
  uint8_t en_channel;
  char en_peer[18];
  char en_pmk[33];
  char en_lmk[33];
  bool en_lr;
  uint8_t en_backlog;
  uint16_t en_wait_ms;
  // Bluetooth
  bool en_ble;
  uint16_t ble_ms;
  bool ble_every;
  uint16_t ble_short_ms;
  char ble_key[33];
  // Administration
  char ap_ssid[33];
  char ap_pass[65];
  uint16_t ap_window_s;
};

enum class FieldType : uint8_t { U8, U16, U32, F32, BOOL, STR, HEXKEY, MAC };

struct Field {
  const char *key;  // clé NVS et nom de champ HTTP, ≤ 15 caractères
  FieldType type;
  uint16_t offset;
  uint16_t size;    // taille du tampon pour STR / HEXKEY / MAC
  float min, max;
  const char *def;  // valeur par défaut, en texte
  const char *group;
  const char *label;
  const char *unit;
  bool secret;      // jamais relu par l'API
};

extern Config cfg;
extern const Field kFields[];
extern const size_t kFieldCount;

void configDefaults(Config &c);
void configLoad();
bool configSave();
const Field *configFind(const char *key);
// Affecte une valeur texte ; rend false si hors bornes ou mal formée.
bool configSet(const char *key, const char *value);
String configGet(const Field &f);
String configJson();  // schéma + valeurs pour la page web

// Utilitaires
bool parseMac(const char *s, uint8_t mac[6]);
bool parseHexKey(const char *s, uint8_t key[16]);  // 32 caractères hex, sinon faux
