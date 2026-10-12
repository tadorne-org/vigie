#include "config.h"

#include <Preferences.h>

#include "board.h"

Config cfg;

#define F_(k, t, member, mn, mx, def, grp, lbl, unit) \
  {k, FieldType::t, offsetof(Config, member), sizeof(Config::member), mn, mx, def, grp, lbl, unit, false}
#define S_(k, t, member, def, grp, lbl, secret) \
  {k, FieldType::t, offsetof(Config, member), sizeof(Config::member), 0, 0, def, grp, lbl, "", secret}

#define STR_(x) #x
#define XSTR_(x) STR_(x)

// Valeurs par défaut de protection et d'énergie.
const Field kFields[] = {
    S_("name", STR, name, "vigie", "Identité", "Nom du nœud", false),
    F_("node_id", U16, node_id, 1, 65535, "40961", "Identité", "Identifiant TsubameBus (src_id)", ""),
    F_("mode", U8, mode, 0, 3, "1", "Identité", "Mode : 0 nav, 1 port, 2 mouillage, 3 hivernage", ""),

    F_("sample_s", U16, sample_s, 5, 3600, "300", "Cadences", "Période de mesure (réveil)", "s"),
    F_("log_s", U16, log_s, 10, 3600, "300", "Cadences", "Période d'enregistrement flash", "s"),
    F_("push_s", U32, push_s, 10, 86400, "900", "Cadences", "Période de push radio", "s"),
    F_("push_winter_s", U32, push_winter_s, 60, 86400, "21600", "Cadences", "Période de push en hivernage", "s"),

    F_("cap_ah", F32, cap_ah, 1, 1000, "73", "Batterie", "Capacité nominale", "Ah"),
    F_("peukert", F32, peukert, 1, 1.6, "1.25", "Batterie", "Exposant de Peukert", ""),
    F_("eta_chg", F32, eta_chg, 0.5, 1, "0.90", "Batterie", "Rendement de charge", ""),
    F_("v_full", F32, v_full, 12, 15.5, "14.2", "Batterie", "Tension « pleine » (resynchro 100 %)", "V"),
    F_("i_tail", F32, i_tail, 0.1, 10, "1.5", "Batterie", "Courant de fin de charge", "A"),
    F_("full_hold_s", U16, full_hold_s, 60, 7200, "600", "Batterie", "Durée de maintien avant resynchro", "s"),
    F_("ocv_rest_s", U32, ocv_rest_s, 0, 172800, "14400", "Batterie", "Repos avant recalage sur tension à vide (0 = jamais)", "s"),

    F_("soc_warn", F32, soc_warn, 0, 100, "60", "Seuils", "Alerte SOC", "%"),
    F_("soc_alarm", F32, soc_alarm, 0, 100, "50", "Seuils", "Alarme SOC", "%"),
    F_("v_low", F32, v_low, 10, 14, "12.0", "Seuils", "Tension basse", "V"),
    F_("v_crit", F32, v_crit, 10, 14, "11.8", "Seuils", "Tension critique", "V"),
    F_("t_high", F32, t_high, 0, 100, "50", "Seuils", "Température haute", "°C"),
    F_("i_over", F32, i_over, 1, 110, "80", "Seuils", "Surintensité", "A"),
    F_("v_throttle", F32, v_throttle, 10, 14, "12.2", "Seuils", "Sous ce seuil : push ÷ 2", "V"),
    F_("v_survival", F32, v_survival, 10, 14, "11.9", "Seuils", "Sous ce seuil : mode survie, push / 6 h", "V"),
    F_("v_cutoff", F32, v_cutoff, 10, 14, "11.5", "Seuils", "Coupure : alerte finale puis silence radio", "V"),
    F_("v_restart", F32, v_restart, 10, 15, "12.6", "Seuils", "Sortie de coupure", "V"),

    F_("sensor", U8, sensor, 0, 2, XSTR_(0), "Capteur", "Source : 0 auto, 1 INA226, 2 simulateur", ""),
    F_("shunt_mohm", F32, shunt_mohm, 0.01, 100, "0.75", "Capteur", "Résistance du shunt", "mΩ"),
    F_("i_gain", F32, i_gain, 0.5, 1.5, "1.0", "Capteur", "Correction de gain courant", ""),
    F_("i_offset_mA", F32, i_offset_mA, -2000, 2000, "0", "Capteur", "Correction de zéro courant", "mA"),
    F_("i_invert", BOOL, i_invert, 0, 1, "0", "Capteur", "Inverser le signe (J2 câblé à l'envers)", ""),
    F_("ina_cont", BOOL, ina_cont, 0, 1, "1", "Capteur", "INA226 en continu (Ah exacts, ~0,33 mA) sinon déclenché", ""),
    F_("sim_scn", U8, sim_scn, 0, 4, "0", "Capteur", "Scénario simulé : 0 port, 1 nav, 2 hivernage, 3 décharge, 4 quai", ""),

    F_("en_espnow", BOOL, en_espnow, 0, 1, "1", "ESP-NOW", "Push ESP-NOW", ""),
    F_("en_channel", U8, en_channel, 1, 13, "1", "ESP-NOW", "Canal Wi-Fi (1 partout)", ""),
    S_("en_peer", MAC, en_peer, "FF:FF:FF:FF:FF:FF", "ESP-NOW", "Destinataire (FF:… = diffusion)", false),
    S_("en_pmk", HEXKEY, en_pmk, "", "ESP-NOW", "Clé PMK (32 hex, vide = aucune)", true),
    S_("en_lmk", HEXKEY, en_lmk, "", "ESP-NOW", "Clé LMK du pair (32 hex, chiffre l'unicast)", true),
    F_("en_lr", BOOL, en_lr, 0, 1, "0", "ESP-NOW", "Mode longue portée (LR, ESP32 seulement)", ""),
    F_("en_backlog", U8, en_backlog, 0, 60, "10", "ESP-NOW", "Trames d'historique max par push", ""),
    F_("en_wait_ms", U16, en_wait_ms, 0, 1000, "60", "ESP-NOW", "Attente d'accusé", "ms"),

    F_("en_ble", BOOL, en_ble, 0, 1, "1", "Bluetooth", "Annonce BTHome v2", ""),
    F_("ble_ms", U16, ble_ms, 200, 10000, "1100", "Bluetooth", "Durée d'annonce par push", "ms"),
    F_("ble_every", BOOL, ble_every, 0, 1, "1", "Bluetooth", "Annoncer aussi à chaque mesure (sans Wi-Fi)", ""),
    F_("ble_short_ms", U16, ble_short_ms, 100, 2000, "300", "Bluetooth", "Durée de l'annonce brève", "ms"),
    S_("ble_key", HEXKEY, ble_key, "", "Bluetooth", "Clé BTHome (32 hex, vide = en clair)", true),

    S_("ap_ssid", STR, ap_ssid, "", "Administration", "Nom du point d'accès (vide = Vigie-XXXX)", false),
    S_("ap_pass", STR, ap_pass, "vigie-admin", "Administration", "Mot de passe du point d'accès (≥ 8)", true),
    F_("ap_window_s", U16, ap_window_s, 30, 3600, "300", "Administration", "Durée d'ouverture après démarrage", "s"),
};
const size_t kFieldCount = sizeof(kFields) / sizeof(kFields[0]);

static void *ptr(Config &c, const Field &f) { return (uint8_t *)&c + f.offset; }

static bool hexNibble(char ch, uint8_t &v) {
  if (ch >= '0' && ch <= '9') v = ch - '0';
  else if (ch >= 'a' && ch <= 'f') v = ch - 'a' + 10;
  else if (ch >= 'A' && ch <= 'F') v = ch - 'A' + 10;
  else return false;
  return true;
}

bool parseMac(const char *s, uint8_t mac[6]) {
  for (int i = 0; i < 6; i++) {
    uint8_t h, l;
    if (!hexNibble(s[0], h) || !hexNibble(s[1], l)) return false;
    mac[i] = h << 4 | l;
    s += 2;
    if (i < 5) {
      if (*s != ':' && *s != '-') return false;
      s++;
    }
  }
  return *s == 0;
}

bool parseHexKey(const char *s, uint8_t key[16]) {
  if (strlen(s) != 32) return false;
  for (int i = 0; i < 16; i++) {
    uint8_t h, l;
    if (!hexNibble(s[2 * i], h) || !hexNibble(s[2 * i + 1], l)) return false;
    key[i] = h << 4 | l;
  }
  return true;
}

static bool setField(Config &c, const Field &f, const char *v) {
  void *p = ptr(c, f);
  char *end;
  switch (f.type) {
    case FieldType::BOOL: {
      bool b = !strcmp(v, "1") || !strcasecmp(v, "true") || !strcasecmp(v, "on");
      if (!b && strcmp(v, "0") && strcasecmp(v, "false") && strcasecmp(v, "off") && *v) return false;
      *(bool *)p = b;
      return true;
    }
    case FieldType::U8:
    case FieldType::U16:
    case FieldType::U32: {
      unsigned long n = strtoul(v, &end, 0);
      if (end == v || *end || n < f.min || n > f.max) return false;
      if (f.type == FieldType::U8) *(uint8_t *)p = n;
      else if (f.type == FieldType::U16) *(uint16_t *)p = n;
      else *(uint32_t *)p = n;
      return true;
    }
    case FieldType::F32: {
      float x = strtof(v, &end);
      if (end == v || *end || !(x >= f.min && x <= f.max)) return false;
      *(float *)p = x;
      return true;
    }
    case FieldType::MAC: {
      uint8_t mac[6];
      if (!parseMac(v, mac)) return false;
      snprintf((char *)p, f.size, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      return true;
    }
    case FieldType::HEXKEY: {
      uint8_t k[16];
      if (*v && !parseHexKey(v, k)) return false;
      strlcpy((char *)p, v, f.size);
      return true;
    }
    case FieldType::STR:
      if (strlen(v) >= f.size) return false;
      if (!strcmp(f.key, "ap_pass") && strlen(v) < 8) return false;  // WPA2
      strlcpy((char *)p, v, f.size);
      return true;
  }
  return false;
}

String configGet(const Field &f) {
  const void *p = (const uint8_t *)&cfg + f.offset;
  switch (f.type) {
    case FieldType::BOOL: return *(const bool *)p ? "1" : "0";
    case FieldType::U8: return String(*(const uint8_t *)p);
    case FieldType::U16: return String(*(const uint16_t *)p);
    case FieldType::U32: return String(*(const uint32_t *)p);
    case FieldType::F32: {
      char b[24];
      snprintf(b, sizeof b, "%g", *(const float *)p);
      return b;
    }
    default: return String((const char *)p);
  }
}

void configDefaults(Config &c) {
  memset(&c, 0, sizeof c);
  for (size_t i = 0; i < kFieldCount; i++) setField(c, kFields[i], kFields[i].def);
  c.sensor = board::kDefaultSensor;
}

const Field *configFind(const char *key) {
  for (size_t i = 0; i < kFieldCount; i++)
    if (!strcmp(kFields[i].key, key)) return &kFields[i];
  return nullptr;
}

void configLoad() {
  configDefaults(cfg);
  Preferences p;
  if (!p.begin("cfg", false)) return;
  for (size_t i = 0; i < kFieldCount; i++) {
    const Field &f = kFields[i];
    if (!p.isKey(f.key)) continue;
    String v = p.getString(f.key, f.def);
    Config tmp = cfg;
    if (setField(tmp, f, v.c_str())) cfg = tmp;  // valeur stockée invalide : défaut conservé
  }
  p.end();
}

bool configSave() {
  Preferences p;
  if (!p.begin("cfg", false)) return false;
  bool ok = true;
  for (size_t i = 0; i < kFieldCount; i++) {
    const Field &f = kFields[i];
    String v = configGet(f);
    if (p.isKey(f.key) && p.getString(f.key) == v) continue;  // inchangé : pas d'écriture flash
    ok &= p.putString(f.key, v) > 0 || v.isEmpty();
  }
  p.end();
  return ok;
}

bool configSet(const char *key, const char *value) {
  const Field *f = configFind(key);
  if (!f) return false;
  Config tmp = cfg;
  if (!setField(tmp, *f, value)) return false;
  cfg = tmp;
  return true;
}

static void jsonEscape(String &out, const char *s) {
  out += '"';
  for (; *s; s++) {
    if (*s == '"' || *s == '\\') { out += '\\'; out += *s; }
    else if ((uint8_t)*s < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", *s); out += b; }
    else out += *s;
  }
  out += '"';
}

String configJson() {
  static const char *types[] = {"u8", "u16", "u32", "f32", "bool", "str", "hexkey", "mac"};
  String o;
  o.reserve(6000);
  o += "[";
  for (size_t i = 0; i < kFieldCount; i++) {
    const Field &f = kFields[i];
    if (i) o += ',';
    o += "{\"k\":\"";
    o += f.key;
    o += "\",\"t\":\"";
    o += types[(int)f.type];
    o += "\",\"g\":";
    jsonEscape(o, f.group);
    o += ",\"l\":";
    jsonEscape(o, f.label);
    o += ",\"u\":";
    jsonEscape(o, f.unit);
    if (f.type <= FieldType::F32) {
      o += ",\"min\":";
      o += String(f.min, 3);
      o += ",\"max\":";
      o += String(f.max, 3);
    }
    if (f.secret) {
      // On dit seulement si une valeur est définie.
      o += ",\"secret\":true,\"set\":";
      o += *((const char *)&cfg + f.offset) ? "true" : "false";
      if (!strcmp(f.key, "ap_pass")) {
        o += ",\"default\":";
        o += strcmp(cfg.ap_pass, f.def) ? "false" : "true";
      }
    } else {
      o += ",\"v\":";
      jsonEscape(o, configGet(f).c_str());
    }
    o += '}';
  }
  o += "]";
  return o;
}
