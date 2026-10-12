#include "sensor.h"

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "battery.h"
#include "board.h"
#include "config.h"
#include "state.h"

// ---------------------------------------------------------------------------
// INA226 (TI SBOS547) — shunt 100 A / 75 mV dans le bus négatif.
// Le courant est calculé depuis la tension de shunt brute (LSB 2,5 µV) plutôt
// que par le registre CURRENT : la correction de gain et de zéro reste ainsi
// entièrement logicielle et le registre de calibration n'a pas d'effet.
// ---------------------------------------------------------------------------
namespace ina {

enum Reg : uint8_t { CONFIG = 0x00, SHUNT = 0x01, BUS = 0x02, MASK = 0x06, LIMIT = 0x07, MFG = 0xFE, DIE = 0xFF };

// Continu : moyenne 1024, VSHCT 8,244 ms, VBUSCT 1,1 ms → fenêtre de 9,6 s.
// La fenêtre glisse pendant que l'ESP32 dort : chaque lecture est une vraie
// moyenne et non un instantané.
constexpr uint16_t kCfgContinuous = 0x4000 | (7 << 9) | (4 << 6) | (7 << 3) | 7;
// Déclenché : moyenne 128, 1,1 ms / 1,1 ms, shunt + bus → 282 ms, puis repos (~2 µA).
constexpr uint16_t kCfgTriggered = 0x4000 | (4 << 9) | (4 << 6) | (4 << 3) | 3;
// Coupure : bus seul en continu, pour l'alerte de remontée de tension.
constexpr uint16_t kCfgBusOnly = 0x4000 | (2 << 9) | (7 << 6) | (4 << 3) | 6;

constexpr uint16_t M_SOL = 1 << 15, M_SUL = 1 << 14, M_BOL = 1 << 13, M_CVRF = 1 << 3, M_LEN = 1 << 0;

bool write(uint8_t reg, uint16_t v) {
  Wire.beginTransmission(board::kInaAddr);
  Wire.write(reg);
  Wire.write(v >> 8);
  Wire.write(v & 0xFF);
  return Wire.endTransmission() == 0;
}

bool read(uint8_t reg, uint16_t &v) {
  Wire.beginTransmission(board::kInaAddr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)board::kInaAddr, 2) != 2) return false;
  v = Wire.read() << 8;
  v |= Wire.read();
  return true;
}

bool probe() {
  uint16_t mfg = 0, die = 0;
  return read(MFG, mfg) && read(DIE, die) && mfg == 0x5449 && (die & 0xFFF0) == 0x2260;
}

bool waitReady(uint32_t timeout_ms) {
  uint32_t t0 = millis();
  uint16_t m;
  while (millis() - t0 < timeout_ms) {
    if (read(MASK, m) && (m & M_CVRF)) return true;
    delay(5);
  }
  return false;
}

bool sample(float &v, float &i) {
  uint16_t sh, bus;
  if (!read(SHUNT, sh) || !read(BUS, bus)) return false;
  float vsh = (int16_t)sh * 2.5e-6f;
  i = vsh / (cfg.shunt_mohm * 1e-3f) * cfg.i_gain + cfg.i_offset_mA * 1e-3f;
  if (cfg.i_invert) i = -i;
  v = bus * 1.25e-3f;
  return true;
}

}  // namespace ina

// ---------------------------------------------------------------------------
// Simulateur. Batterie plomb, capacité cfg.cap_ah, avec :
//  - panneau 100 W à plat : ensoleillement selon l'heure et la saison, nébulosité
//    en marche aléatoire ;
//  - acceptation de charge décroissante près de 100 % (absorption puis floating),
//    ce qui déclenche la resynchronisation du SOC comme en vrai ;
//  - charges du bord selon le scénario (soirées à bord, instruments, pilote,
//    moteur, chargeur de quai).
// L'état vit en RTC RAM : la simulation continue pendant le sommeil profond et
// rattrape le temps écoulé au réveil. Toute mesure simulée porte le drapeau SIM.
// ---------------------------------------------------------------------------
namespace sim {

enum Scn : uint8_t { PORT = 0, NAV = 1, WINTER = 2, DISCHARGE = 3, SHORE = 4 };
constexpr float kRint = 0.018f;

SimState &S = rtc.sim_state;

uint32_t rnd() {
  uint32_t x = S.rng ? S.rng : 0x9E3779B9;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return S.rng = x;
}
float uniform() { return (rnd() >> 8) * (1.0f / 16777216.0f); }
float gauss() {
  float u1 = uniform() + 1e-7f, u2 = uniform();
  return sqrtf(-2 * logf(u1)) * cosf(2 * PI * u2);
}

// Heure locale approximative (UTC+1) ; sans horloge, on part d'un 21 juin 10 h.
void localTime(uint32_t t, float &hour, int &doy, int &dow, uint16_t &day) {
  int64_t lt = nodeclock::valid() ? (int64_t)t + 3600 : (int64_t)t + 10 * 3600 + 171 * 86400;
  day = lt / 86400;
  hour = (lt % 86400) / 3600.0f;
  doy = day % 365;
  dow = (day + 3) % 7;  // 1970-01-01 = jeudi ; 0 = lundi
}

float ocv(double soc) { return ocvFromSoc((float)soc); }

// Tension aux bornes : résistance interne, plus en charge une surtension qui
// s'établit vite (~0,8 V) et une montée en fin de charge.
float terminal(double soc, float i) {
  float v = ocv(soc) + i * kRint;
  if (i > 0) v += 0.8f * (1 - expf(-i / 4)) + 0.9f * powf((float)soc / 100, 6) * fminf(1, i / 3);
  return v;
}

struct Flow {
  float src;     // courant offert par les sources (A)
  float load;    // consommation (A)
  float v_reg;   // tension de régulation des sources, 0 = pas de régulateur
};

Flow flows(uint32_t t) {
  float h;
  int doy, dow;
  uint16_t day;
  localTime(t, h, doy, dow, day);
  uint8_t scn = cfg.sim_scn;
  if (scn == WINTER) doy = 15;

  if (day != S.day_id) {  // tirages du jour
    S.day_id = day;
    S.day_evening = uniform() < (scn == NAV ? 0.9f : 0.25f);
  }
  if (t - S.cloud_t >= 3600 || S.cloud_t == 0 || t < S.cloud_t) {  // nébulosité horaire
    S.cloud_t = t;
    S.cloud = fminf(1.0f, fmaxf(0.15f, S.cloud + (uniform() - 0.5f) * 0.5f));
  }

  Flow f{0, 0.004f, 0};  // la carte Vigie elle-même, mesurée par le shunt

  // Soleil
  float daylen = 12 + 4 * sinf(2 * PI * (doy - 80) / 365.0f);
  float rise = 13.2f - daylen / 2;  // midi solaire ≈ 13 h 12 en UTC+1 (ouest de la France)
  float x = (h - rise) / daylen;
  float irr = (x > 0 && x < 1) ? powf(sinf(PI * x), 1.5f) : 0;
  float season = 0.55f + 0.45f * sinf(2 * PI * (doy - 80) / 365.0f);
  float pv = 6.2f * irr * season * S.cloud;
  if (scn == WINTER) pv *= 0.35f;
  if (scn == DISCHARGE) pv = 0;
  if (pv > 0.01f) {
    f.src += pv;
    f.v_reg = 14.4f;
  }

  bool weekend = dow >= 5;
  switch (scn) {
    case PORT:
    case SHORE:
      if (S.day_evening && h >= 19 && h < 23) f.load += 0.8f;
      if (weekend && h >= 10 && h < 18) f.load += 0.3f;
      break;
    case NAV:
      if (h >= 8 && h < 19) {
        f.load += 0.35f;                                   // B&G + tableau de bord
        if (((int)(h * 6)) % 4 == 0) f.load += 1.5f;       // pilote de barre
      }
      if ((h >= 8.5f && h < 9.25f) || (h >= 18 && h < 18.75f)) {
        f.src += 5.0f;                                     // hors-bord
        f.v_reg = fmaxf(f.v_reg, 14.2f);
      }
      if (h >= 21 || h < 6) f.load += 0.5f;                // feux
      if (S.day_evening && h >= 20 && h < 23) f.load += 0.8f;
      break;
    case WINTER:
      break;
    case DISCHARGE:
      f.load += 4.0f;
      break;
  }
  if (scn == SHORE) {
    f.src += 15.0f;  // Blue Smart IP65 12/15
    f.v_reg = 14.4f;
  }
  // Floating après deux heures pleines
  if (f.v_reg > 0 && S.full_since && t - S.full_since > 7200) f.v_reg = scn == SHORE ? 13.6f : 13.7f;
  return f;
}

// Courant accepté par la batterie en absorption.
float acceptance(double soc) {
  float k = fminf(1.0f, fmaxf(0.0f, (100.5f - (float)soc) / 12.0f));
  return fmaxf(0.15f, cfg.cap_ah * 0.25f * k);
}

void step(uint32_t t, float dt, float &v, float &i) {
  Flow f = flows(t);
  float net = f.src - f.load;
  bool regulating = false;
  if (f.v_reg > 0 && net > 0) {
    float acc = acceptance(S.soc);
    float v_free = terminal(S.soc, net);
    if (v_free >= f.v_reg || net > acc) {
      regulating = true;
      net = fminf(net, acc);
      if (f.v_reg < 14.0f) net = fminf(net, 0.15f);  // floating : entretien seulement
      v = f.v_reg;
    }
  }
  i = net;
  if (!regulating) v = terminal(S.soc, i);
  // Intégration du SOC « vrai »
  double di = i;
  if (i > 0) di *= 0.9;                               // rendement réel du plomb
  else if (-i > cfg.cap_ah / 20) di *= pow(-i / (cfg.cap_ah / 20), 0.2);  // Peukert
  S.soc += di * dt / 3600.0 / cfg.cap_ah * 100.0;
  S.soc -= dt * (3.0 / (30 * 86400.0));               // auto-décharge ~3 %/mois
  S.soc = fmin(100.0, fmax(0.0, S.soc));
  if (S.soc >= 99.9) {
    if (!S.full_since) S.full_since = t;
  } else if (S.soc < 97) {
    S.full_since = 0;
  }
}

float temperature(uint32_t t) {
  float h;
  int doy, dow;
  uint16_t day;
  localTime(t, h, doy, dow, day);
  static const float base[] = {17, 19, 6, 18, 16};
  return base[cfg.sim_scn % 5] + 5 * sinf(2 * PI * (h - 9) / 24) + 0.2f * gauss();
}

bool read(Reading &r) {
  uint32_t now = nodeclock::now();
  if (S.last_t == 0 || now < S.last_t || now - S.last_t > 7 * 86400) S.last_t = now;  // horloge recalée : pas de rattrapage fantôme
  float v = ocv(S.soc), i = 0;
  // Rattrapage du temps écoulé par pas de 60 s au plus
  uint32_t t = S.last_t;
  if (now == t) step(t, 0, v, i);
  while (t < now) {
    uint32_t dt = min<uint32_t>(60, now - t);
    step(t, dt, v, i);
    t += dt;
  }
  S.last_t = now;
  r.v = v + 0.003f * gauss();
  r.i = i + 0.008f * gauss();
  r.t = temperature(now);
  r.t_ok = true;
  r.ok = true;
  r.sim = true;
  return true;
}

}  // namespace sim

void simReset(float soc) {
  sim::S.soc = soc;
  sim::S.full_since = 0;
  sim::S.last_t = 0;
}

const char *simScenarioName(uint8_t s) {
  static const char *n[] = {"port", "navigation", "hivernage", "décharge 4 A", "quai (chargeur 15 A)"};
  return s < 5 ? n[s] : "?";
}

// ---------------------------------------------------------------------------

static enum { SRC_NONE, SRC_INA, SRC_SIM } src = SRC_NONE;
static bool ina_first = false;

void sensorBegin(bool cold) {
  if (cold || rtc.sim_state.rng == 0) {
    if (rtc.sim_state.rng == 0) {
      rtc.sim_state.rng = esp_random() | 1;
      rtc.sim_state.soc = 85;
      rtc.sim_state.cloud = 0.7f;
    }
  }
  src = SRC_SIM;
  if (cfg.sensor == 2) return;
  Wire.begin(board::kSda, board::kScl, 400000);
  Wire.setTimeOut(20);
  if (!ina::probe()) {
    if (cfg.sensor == 1) src = SRC_NONE;  // INA226 imposé mais muet : erreur capteur
    return;
  }
  src = SRC_INA;
  uint16_t c = 0;
  ina::read(ina::CONFIG, c);
  uint16_t want = cfg.ina_cont ? ina::kCfgContinuous : ina::kCfgTriggered;
  // Premier démarrage ou INA réinitialisé (baisse de tension) : on reconfigure.
  // En continu, la première fenêtre de 9,6 s n'est pas encore pleine : une
  // conversion déclenchée courte donne une première valeur tout de suite.
  if (cold || (cfg.ina_cont && c != want)) {
    ina::write(ina::CONFIG, ina::kCfgTriggered);
    ina_first = cfg.ina_cont;
  }
}

bool sensorRead(Reading &r) {
  r = Reading{};
  switch (src) {
    case SRC_SIM:
      return sim::read(r);
    case SRC_INA: {
      if (!cfg.ina_cont || ina_first) {
        if (!ina_first) ina::write(ina::CONFIG, ina::kCfgTriggered);
        if (!ina::waitReady(500)) return false;
      }
      r.ok = ina::sample(r.v, r.i);
      if (ina_first) {
        ina::write(ina::CONFIG, ina::kCfgContinuous);
        ina_first = false;
      }
      return r.ok;
    }
    default:
      return false;
  }
}

const char *sensorName() {
  switch (src) {
    case SRC_INA: return cfg.ina_cont ? "INA226 (continu)" : "INA226 (déclenché)";
    case SRC_SIM: return "simulateur";
    default: return "aucun (INA226 muet)";
  }
}

bool sensorIsSim() { return src == SRC_SIM; }

bool sensorSleep(float alert_over_v, float alert_over_a) {
  if (src != SRC_INA) return false;
  uint16_t dummy;
  ina::read(ina::MASK, dummy);  // acquitte une alerte verrouillée
  if (alert_over_v > 0) {
    ina::write(ina::CONFIG, ina::kCfgBusOnly);
    ina::write(ina::LIMIT, (uint16_t)(alert_over_v / 1.25e-3f));
    ina::write(ina::MASK, ina::M_BOL | ina::M_LEN);
    return true;
  }
  if (cfg.ina_cont && alert_over_a > 0) {
    // Surintensité de décharge. Sans inversion, décharge = tension de shunt négative.
    float lsb = alert_over_a * cfg.shunt_mohm * 1e-3f / 2.5e-6f / cfg.i_gain;
    int16_t lim = (int16_t)fminf(32767, lsb);
    ina::write(ina::LIMIT, (uint16_t)(cfg.i_invert ? lim : -lim));
    ina::write(ina::MASK, (cfg.i_invert ? ina::M_SOL : ina::M_SUL) | ina::M_LEN);
    return true;
  }
  ina::write(ina::MASK, 0);
  if (!cfg.ina_cont) ina::write(ina::CONFIG, 0x4000 | 0);  // arrêt : ~2 µA
  return false;
}
