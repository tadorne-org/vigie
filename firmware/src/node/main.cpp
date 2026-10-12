// Vigie — firmware du nœud énergie.
//
// Deux régimes :
//  - cycle de sommeil : réveil → mesure → (enregistrement) → (push radio) →
//    sommeil profond. C'est le régime normal, qui tient la batterie l'hiver.
//  - éveillé : après un démarrage à froid ou un appui sur BOOT, point d'accès
//    Wi-Fi et page d'administration pendant cfg.ap_window_s, mesures et push
//    continuant au même rythme ; puis retour au cycle de sommeil.
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_timer.h>

#include "app.h"
#include "battery.h"
#include "board.h"
#include "config.h"
#include "flashlog.h"
#include "radio.h"
#include "record.h"
#include "sensor.h"
#include "state.h"
#include "tbus.h"

RTC_DATA_ATTR RtcState rtc;

static Reading g_last;
static bool g_awake = false;
static bool g_force_push = false;

// ---------------------------------------------------------------------------
// Horloge

int64_t nodeclock::set(int64_t epoch, const char *source) {
  int64_t before = nowUs();
  bool was_valid = valid();
  struct timeval tv = {(time_t)epoch, 0};
  settimeofday(&tv, nullptr);
  int64_t delta_us = nowUs() - before;
  int32_t d = (int32_t)(delta_us / 1000000);

  // Instants toujours définis (0 est une vraie date avant recalage)…
  auto sh = [d](uint32_t &x) { x += d; };
  sh(rtc.boot_t); sh(rtc.agg.start); sh(rtc.day.start);
  sh(rtc.next_sample); sh(rtc.next_log); sh(rtc.next_day);
  if (rtc.next_push != UINT32_MAX) sh(rtc.next_push);
  // …et instants où 0 signifie « jamais ».
  auto shSet = [d](uint32_t &x) { if (x) x += d; };
  shSet(rtc.last_sample); shSet(rtc.full_since); shSet(rtc.rest_since); shSet(rtc.last_charge); shSet(rtc.last_ack);
  for (auto &a : rtc.alert_last) shSet(a);
  shSet(rtc.sim_state.last_t); shSet(rtc.sim_state.cloud_t); shSet(rtc.sim_state.full_since);
  if (rtc.last_sample_us) rtc.last_sample_us += delta_us;
  if (!was_valid) {
    // Première heure vraie : la journée en cours se termine à minuit UTC.
    rtc.next_day = (uint32_t)((epoch / 86400 + 1) * 86400);
  }

  vlog::Record r = {};
  r.ts = now();
  r.type_mode = vlog::REC_TIME_SYNC | (cfg.mode << 4);
  r.i_avg_mA = d;
  r.flags = vlog::RF_TIME_VALID | (rtc.sim ? vlog::RF_SIM : 0);
  r.t_cC = vlog::kNoTemp;
  flashlog::samples.append(&r);
  Serial.printf("[clock] recalée par %s : %+ld s\n", source, (long)d);
  return d;
}

// ---------------------------------------------------------------------------
// Persistance des compteurs (NVS) — survit à une coupure d'alimentation.

static void persistStats() {
  Preferences p;
  if (!p.begin("stats", false)) return;
  p.putDouble("ah_in", rtc.ah_in);
  p.putDouble("ah_out", rtc.ah_out);
  p.putDouble("q", rtc.q_C);
  p.putUInt("acked", rtc.acked_rec);
  p.putUInt("lastchg", rtc.last_charge);
  p.end();
}

static void restoreStats() {
  Preferences p;
  if (!p.begin("stats", false)) return;
  if (p.isKey("ah_in")) {  // absent au tout premier démarrage
    rtc.ah_in = p.getDouble("ah_in", 0);
    rtc.ah_out = p.getDouble("ah_out", 0);
    rtc.q_C = p.getDouble("q", 0);
    rtc.acked_rec = p.getUInt("acked", 0);
    rtc.last_charge = p.getUInt("lastchg", 0);
  }
  rtc.cold_boots = p.getUInt("boots", 0) + 1;
  p.putUInt("boots", rtc.cold_boots);
  p.end();
}

// ---------------------------------------------------------------------------
// Journal

static uint8_t baseFlags() {
  uint8_t f = 0;
  if (rtc.sim) f |= vlog::RF_SIM;
  if (nodeclock::valid()) f |= vlog::RF_TIME_VALID;
  if (!rtc.sensor_ok) f |= vlog::RF_SENSOR_ERR;
  if (rtc.power_state == 2) f |= vlog::RF_SURVIVAL;
  if (rtc.power_state == 3) f |= vlog::RF_CUTOFF;
  return f;
}

static vlog::Record snapshot(vlog::RecType type, uint16_t arg) {
  vlog::Record r = {};
  r.ts = nodeclock::now();
  r.i_avg_mA = lroundf(rtc.i * 1000);
  r.v_avg_mV = r.v_min_mV = r.v_max_mV = lroundf(rtc.v * 1000);
  r.i_min_cA = r.i_max_cA = constrain(lroundf(rtc.i * 100), -32767, 32767);
  r.soc_cp = lroundf(rtc.soc * 100);
  r.t_cC = rtc.t_ok ? lroundf(rtc.t * 100) : vlog::kNoTemp;
  r.dt_s = arg;
  r.type_mode = type | (cfg.mode << 4);
  r.flags = baseFlags();
  return r;
}

static void logEvent(vlog::RecType type, uint16_t arg) {
  vlog::Record r = snapshot(type, arg);
  flashlog::samples.append(&r);
}

static void aggReset(uint32_t now) {
  rtc.agg = Agg{};
  rtc.agg.start = now;
}

static void writeSample(uint32_t now) {
  Agg &a = rtc.agg;
  if (a.n) {
    vlog::Record r = {};
    r.ts = a.start;
    r.i_avg_mA = lround(a.i_sum / a.n * 1000);
    r.v_avg_mV = lround(a.v_sum / a.n * 1000);
    r.v_min_mV = lroundf(a.v_min * 1000);
    r.v_max_mV = lroundf(a.v_max * 1000);
    r.i_min_cA = constrain(lroundf(a.i_min * 100), -32767, 32767);
    r.i_max_cA = constrain(lroundf(a.i_max * 100), -32767, 32767);
    r.soc_cp = lroundf(rtc.soc * 100);
    r.t_cC = a.t_n ? lroundf(a.t_sum / a.t_n * 100) : vlog::kNoTemp;
    r.dt_s = min<uint32_t>(65535, now - a.start);
    r.type_mode = vlog::REC_SAMPLE | (cfg.mode << 4);
    r.flags = baseFlags() | a.flags;
    if (!flashlog::samples.append(&r)) Serial.println("[vlog] écriture impossible");
  }
  aggReset(now);
  if (++rtc.logs_since_persist >= max(1, 3600 / cfg.log_s)) {
    rtc.logs_since_persist = 0;
    persistStats();
  }
}

static void dayReset(uint32_t now) {
  rtc.day = DayAgg{};
  rtc.day.start = now;
  rtc.day.v_min = rtc.day.soc_min = rtc.day.t_min = 1e9f;
  rtc.day.v_max = rtc.day.soc_max = rtc.day.t_max = -1e9f;
}

static void writeDay(uint32_t now) {
  DayAgg &d = rtc.day;
  if (d.samples) {
    vlog::DayRecord r = {};
    r.ts = d.start;
    r.mah_in = lround(d.mah_in);
    r.mah_out = lround(d.mah_out);
    r.v_min_mV = lroundf(d.v_min * 1000);
    r.v_max_mV = lroundf(d.v_max * 1000);
    r.soc_min_cp = lroundf(d.soc_min * 100);
    r.soc_max_cp = lroundf(d.soc_max * 100);
    r.t_min_cC = d.t_max > -1e8f ? lroundf(d.t_min * 100) : vlog::kNoTemp;
    r.t_max_cC = d.t_max > -1e8f ? lroundf(d.t_max * 100) : vlog::kNoTemp;
    r.flags = d.flags | (nodeclock::valid() ? vlog::RF_TIME_VALID : 0);
    r.hours = min<uint32_t>(255, (now - d.start + 1800) / 3600);
    flashlog::days.append(&r);
  }
  dayReset(now);
  persistStats();
}

// ---------------------------------------------------------------------------
// Alertes et protection de la batterie 

static void raise(uint16_t code, bool cond, bool clear_cond, uint32_t now, uint32_t repeat_s = 0) {
  uint32_t bit = 1u << code;
  if (cond) {
    bool fresh = !(rtc.alert_active & bit);
    bool again = repeat_s && now - rtc.alert_last[code] >= repeat_s;
    if (fresh || again) {
      rtc.alert_active |= bit;
      rtc.alert_pending |= bit;
      rtc.alert_last[code] = now;
      logEvent(vlog::REC_ALERT, code);
      Serial.printf("[alerte] %s\n", vlog::alertName(code));
    }
  } else if (clear_cond) {
    rtc.alert_active &= ~bit;
  }
}

static void evaluateAlerts(const Reading &r, uint32_t now) {
  using namespace vlog;
  raise(AL_SENSOR_LOST, !r.ok, r.ok, now, 6 * 3600);
  if (!r.ok) return;
  raise(AL_SOC_WARN, rtc.soc < cfg.soc_warn, rtc.soc > cfg.soc_warn + 2, now);
  raise(AL_SOC_ALARM, rtc.soc < cfg.soc_alarm, rtc.soc > cfg.soc_alarm + 2, now, 6 * 3600);
  raise(AL_VOLT_LOW, r.v < cfg.v_low, r.v > cfg.v_low + 0.2f, now);
  raise(AL_VOLT_CRITICAL, r.v < cfg.v_crit, r.v > cfg.v_crit + 0.2f, now, 6 * 3600);
  if (r.t_ok) raise(AL_TEMP_HIGH, r.t > cfg.t_high, r.t < cfg.t_high - 3, now);
  raise(AL_OVERCURRENT, fabsf(r.i) > cfg.i_over, fabsf(r.i) < cfg.i_over * 0.9f, now);
  if (rtc.last_charge)
    raise(AL_NO_CHARGE_30D, now - rtc.last_charge > 30 * 86400, now - rtc.last_charge < 86400, now, 7 * 86400);
}

static uint8_t powerTarget(float v) {
  if (v < cfg.v_cutoff) return 3;
  if (v < cfg.v_survival) return 2;
  if (v < cfg.v_throttle) return 1;
  return 0;
}

static void updatePowerState(const Reading &r, uint32_t now) {
  if (!r.ok) return;
  uint8_t cur = rtc.power_state;
  if (cur == 3) {  // coupure : on n'en sort qu'au-dessus de v_restart (charge détectée)
    if (r.v >= cfg.v_restart) {
      rtc.power_state = powerTarget(r.v);
      rtc.next_push = now;  // la coupure avait suspendu les push : on annonce le rétablissement
      logEvent(vlog::REC_CUTOFF, 0);
      Serial.println("[protection] sortie de coupure");
    }
    return;
  }
  uint8_t tgt = powerTarget(r.v);
  if (tgt > cur) {
    // Deux évaluations de suite : un démarrage moteur ne doit pas couper le nœud.
    if (++rtc.low_count < 2) return;
    rtc.power_state = tgt;
    if (tgt == 3) {
      logEvent(vlog::REC_CUTOFF, 1);
      raise(vlog::AL_CUTOFF, true, false, now);
      g_force_push = true;  // alerte finale
      Serial.println("[protection] COUPURE sous-tension");
    }
  } else if (tgt < cur && r.v > (cur == 1 ? cfg.v_throttle : cfg.v_survival) + 0.1f) {
    rtc.power_state = tgt;
  }
  if (tgt <= cur) rtc.low_count = 0;
}

static uint32_t pushPeriod() {
  uint32_t p = cfg.mode == 3 ? cfg.push_winter_s : cfg.push_s;
  switch (rtc.power_state) {
    case 1: return p * 2;
    case 2: return max<uint32_t>(p, 6 * 3600);
    case 3: return UINT32_MAX;
  }
  return p;
}

static uint32_t samplePeriod() {
  return rtc.power_state == 3 ? max<uint32_t>(cfg.sample_s, 3600) : cfg.sample_s;
}

// ---------------------------------------------------------------------------
// Mesure, enregistrement, push

static void doSample(uint32_t now) {
  Reading r;
  bool ok = sensorRead(r);
  int64_t now_us = nodeclock::nowUs();
  float dt = rtc.last_sample_us ? (now_us - rtc.last_sample_us) / 1e6f : 0;
  float soc_before;
  SocEvent ev = batteryUpdate(r, now, dt, soc_before);
  if (ok) {
    rtc.v = r.v;
    rtc.i = r.i;
    rtc.t = r.t;
    rtc.t_ok = r.t_ok;
    rtc.last_sample_us = now_us;
  }
  rtc.sensor_ok = ok;
  rtc.sim = r.sim;
  rtc.last_sample = now;
  g_last = r;

  if (ev == SocEvent::FULL || ev == SocEvent::OCV) {
    logEvent(vlog::REC_SOC_SET, ev == SocEvent::FULL ? vlog::SOC_FULL : vlog::SOC_OCV);
    if (ev == SocEvent::FULL) rtc.agg.flags |= vlog::RF_SOC_SYNC;
    Serial.printf("[soc] recalage %s : %.1f → %.1f %%\n", ev == SocEvent::FULL ? "pleine charge" : "tension à vide",
                  soc_before, rtc.soc);
  }

  if (ok) {
    Agg &a = rtc.agg;
    if (!a.n) {
      a.v_min = a.v_max = r.v;
      a.i_min = a.i_max = r.i;
    }
    a.n++;
    a.v_sum += r.v;
    a.i_sum += r.i;
    a.v_min = fminf(a.v_min, r.v);
    a.v_max = fmaxf(a.v_max, r.v);
    a.i_min = fminf(a.i_min, r.i);
    a.i_max = fmaxf(a.i_max, r.i);
    if (r.t_ok) { a.t_sum += r.t; a.t_n++; }
    if (r.i > cfg.i_tail) a.flags |= vlog::RF_CHARGING;
    if (r.v < cfg.v_low) a.flags |= vlog::RF_UNDERVOLT;

    DayAgg &d = rtc.day;
    d.samples++;
    d.v_min = fminf(d.v_min, r.v);
    d.v_max = fmaxf(d.v_max, r.v);
    d.soc_min = fminf(d.soc_min, rtc.soc);
    d.soc_max = fmaxf(d.soc_max, rtc.soc);
    if (r.t_ok) { d.t_min = fminf(d.t_min, r.t); d.t_max = fmaxf(d.t_max, r.t); }
    if (r.sim) d.flags |= vlog::RF_SIM;
  }
  evaluateAlerts(r, now);
  updatePowerState(r, now);
  if (rtc.alert_pending) g_force_push = true;
}

static void doPush(bool ap_active) {
  rtc.pushes++;
  if (rtc.power_state == 3 && !(rtc.alert_pending & (1u << vlog::AL_CUTOFF))) return;  // silence radio
  digitalWrite(board::kLed, HIGH);
  if (cfg.en_espnow && espnow::begin(ap_active)) {
    if (rtc.hello_pending) {
      espnow::sendHello();
      rtc.hello_pending = false;
    }
    espnow::sendEnergy(g_last);
    for (int code = 0; code < 32; code++) {
      if (!(rtc.alert_pending & (1u << code))) continue;
      uint8_t sev = (code == vlog::AL_SOC_ALARM || code == vlog::AL_VOLT_CRITICAL || code == vlog::AL_OVERCURRENT ||
                     code == vlog::AL_CUTOFF || code == vlog::AL_SENSOR_LOST)
                        ? tbus::SEV_CRIT
                        : tbus::SEV_WARN;
      float val = code == vlog::AL_SOC_WARN || code == vlog::AL_SOC_ALARM ? rtc.soc
                  : code == vlog::AL_TEMP_HIGH                          ? rtc.t
                  : code == vlog::AL_OVERCURRENT                        ? rtc.i
                                                                        : rtc.v;
      float lim = code == vlog::AL_SOC_WARN ? cfg.soc_warn : code == vlog::AL_SOC_ALARM ? cfg.soc_alarm
                  : code == vlog::AL_VOLT_LOW ? cfg.v_low : code == vlog::AL_VOLT_CRITICAL ? cfg.v_crit
                  : code == vlog::AL_TEMP_HIGH ? cfg.t_high : code == vlog::AL_OVERCURRENT ? cfg.i_over
                  : code == vlog::AL_CUTOFF ? cfg.v_cutoff : 0;
      espnow::sendAlert(code, sev, val, lim);
    }
    espnow::poll(cfg.en_wait_ms);
    if (rtc.power_state < 2) espnow::sendBacklog();
    if (!ap_active) espnow::end();
  }
  rtc.alert_pending = 0;
  if (cfg.en_ble && !ap_active) {
    // Sommeil : annonce brève. Éveillé : l'annonce tourne en continu (loop).
    if (bthome::start()) {
      bthome::update(g_last, rtc.soc);
      delay(cfg.ble_ms);
      bthome::stop();
    }
  }
  digitalWrite(board::kLed, LOW);
}

// Exécute ce qui est dû à l'instant `now`.
static void runDue(bool ap_active) {
  uint32_t now = nodeclock::now();
  bool sampled = false;
  if (now >= rtc.next_sample || g_force_push) {
    doSample(now);
    sampled = true;
    rtc.next_sample = now + samplePeriod();
  }
  if (sampled && now >= rtc.next_log) {
    writeSample(now);
    rtc.next_log = now + cfg.log_s;
  }
  if (now >= rtc.next_day) {
    writeDay(now);
    rtc.next_day = nodeclock::valid() ? (now / 86400 + 1) * 86400 : now + 86400;
  }
  bool pushed = false;
  if (now >= rtc.next_push || g_force_push) {
    doPush(ap_active);
    pushed = true;
    uint32_t p = pushPeriod();
    rtc.next_push = p == UINT32_MAX ? UINT32_MAX : now + p;
    g_force_push = false;
  }
  // Annonce BLE brève à chaque réveil de mesure, sans Wi-Fi : l'app iPhone
  // reçoit une valeur par période de mesure au lieu d'une par push. Coupée en
  // survie et en coupure, où chaque milliampère-heure compte.
  if (sampled && !pushed && !ap_active && cfg.en_ble && cfg.ble_every && rtc.power_state < 2 &&
      bthome::start()) {
    bthome::update(g_last, rtc.soc);
    delay(cfg.ble_short_ms);
    bthome::stop();
  }
  if (sampled && ap_active && cfg.en_ble && bthome::running()) bthome::update(g_last, rtc.soc);
}

// ---------------------------------------------------------------------------
// Sommeil

[[noreturn]] static void deepSleep() {
  uint32_t now = nodeclock::now();
  uint32_t next = min(rtc.next_sample, rtc.next_push);
  int64_t us = (int64_t)next * 1000000 - nodeclock::nowUs();
  if (us < 1000000) us = 1000000;

  bool ina_wake = sensorSleep(rtc.power_state == 3 ? cfg.v_restart : 0,
                              (rtc.alert_active & (1u << vlog::AL_OVERCURRENT)) ? 0 : cfg.i_over);
  esp_sleep_enable_timer_wakeup(us);
  if (ina_wake) esp_sleep_enable_ext0_wakeup(board::kInaAlert, 0);
  esp_sleep_enable_ext1_wakeup(1ULL << board::kButton, ESP_EXT1_WAKEUP_ALL_LOW);
  Serial.printf("[sleep] %lld s après %lu ms d'éveil (mesure dans %ld s, push dans %ld s)%s\n", us / 1000000,
                (unsigned long)millis(),
                (long)(rtc.next_sample - now), rtc.next_push == UINT32_MAX ? -1L : (long)(rtc.next_push - now),
                ina_wake ? ", réveil INA226 armé" : "");
  Serial.flush();
  esp_deep_sleep_start();
}

// Filet de sécurité : un cycle de sommeil ne doit jamais rester éveillé.
static void watchdogFire(void *) { esp_deep_sleep_start(); }

// ---------------------------------------------------------------------------
// Démarrage

static void coldInit() {
  memset(&rtc, 0, sizeof rtc);
  rtc.magic = kRtcMagic;
  rtc.boot_t = nodeclock::now();
  restoreStats();
  bthome::initCounter();
  rtc.hello_pending = true;
  uint32_t now = nodeclock::now();
  aggReset(now);
  dayReset(now);
  rtc.next_day = now + 86400;
  rtc.next_log = now + cfg.log_s;

  // SOC : dernier échantillon du journal (il survit à la coupure).
  vlog::Record r;
  uint32_t end = flashlog::samples.nextSeq();
  for (uint32_t s = end; s-- > flashlog::samples.firstSeq() && end - s < 500;) {
    if (!flashlog::samples.read(s, &r)) continue;
    uint8_t t = r.type_mode & 0x0F;
    if (t == vlog::REC_SAMPLE || t == vlog::REC_SOC_SET) {
      rtc.soc = r.soc_cp / 100.0;
      rtc.soc_valid = true;
      rtc.soc_est = false;
      Serial.printf("[soc] restauré depuis l'enregistrement #%u : %.1f %%\n", (unsigned)s, rtc.soc);
      break;
    }
  }

  esp_reset_reason_t why = esp_reset_reason();
  vlog::Record boot = snapshot(vlog::REC_BOOT, why);
  boot.i_avg_mA = rtc.cold_boots;
  boot.flags &= ~vlog::RF_SENSOR_ERR;  // pas encore de mesure : ce n'est pas une panne
  flashlog::samples.append(&boot);
  if (why == ESP_RST_PANIC || why == ESP_RST_INT_WDT || why == ESP_RST_TASK_WDT || why == ESP_RST_WDT ||
      why == ESP_RST_BROWNOUT) {
    rtc.alert_pending |= 1u << vlog::AL_BOOT_UNEXPECTED;
  }
}

void setup() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  bool wake = rtc.magic == kRtcMagic &&
              (cause == ESP_SLEEP_WAKEUP_TIMER || cause == ESP_SLEEP_WAKEUP_EXT0 || cause == ESP_SLEEP_WAKEUP_EXT1);
  Serial.begin(115200);
  pinMode(board::kLed, OUTPUT);
  digitalWrite(board::kLed, LOW);

  configLoad();
  flashlog::begin();

  if (!wake) {
    Serial.printf("\n=== Vigie %s — %s — démarrage à froid ===\n", VIGIE_FW_VERSION, board::kName);
    coldInit();
  } else {
    rtc.wakes++;
    setCpuFrequencyMhz(80);  // assez pour le Wi-Fi, moitié moins gourmand
    const esp_timer_create_args_t a = {.callback = watchdogFire, .arg = nullptr, .dispatch_method = ESP_TIMER_TASK,
                                       .name = "wd", .skip_unhandled_events = true};
    esp_timer_handle_t wd;
    if (esp_timer_create(&a, &wd) == ESP_OK) esp_timer_start_once(wd, (15000 + cfg.ble_ms) * 1000ULL);
  }
  sensorBegin(!wake);

  bool open_admin = !wake;
  if (cause == ESP_SLEEP_WAKEUP_EXT1) open_admin = true;  // bouton BOOT
  if (cause == ESP_SLEEP_WAKEUP_EXT0) g_force_push = true;  // alerte INA226

  if (open_admin) {
    g_awake = true;
    admin::start();
    if (cfg.en_espnow) espnow::begin(true);
    if (cfg.en_ble) bthome::start();
    g_force_push = true;  // première mesure et premier push tout de suite
    Serial.println("Console série : taper « help ».");
    return;
  }
  runDue(false);
  deepSleep();
}

void loop() {
  // N'est atteint qu'en régime éveillé.
  admin::loop();
  cli::poll();
  runDue(true);
  static uint32_t blink = 0;
  if (millis() - blink > 1000) {
    blink = millis();
    digitalWrite(board::kLed, HIGH);
    delay(15);
    digitalWrite(board::kLed, LOW);
  }
  if (!admin::active()) {
    Serial.println("[admin] fermeture, retour au cycle de sommeil");
    bthome::stop();
    espnow::end();
    admin::stop();
    persistStats();
    deepSleep();
  }
  delay(5);
}

// ---------------------------------------------------------------------------
// Interface pour l'administration et la console

namespace app {

const Reading &last() { return g_last; }
void sampleNow() { rtc.next_sample = 0; }
void pushNow() { g_force_push = true; }

void setSoc(float soc) {
  batterySetSoc(soc);
  logEvent(vlog::REC_SOC_SET, vlog::SOC_MANUAL);
}

bool eraseLogs() {
  bool ok = flashlog::samples.erase() && flashlog::days.erase();
  rtc.acked_rec = flashlog::samples.nextSeq() - 1;
  return ok;
}

void sleepNow() { admin::close(); }

void configChanged() {
  logEvent(vlog::REC_CONFIG, 0);
  sensorBegin(false);
  uint32_t now = nodeclock::now();
  rtc.next_sample = min(rtc.next_sample, now + cfg.sample_s);
  rtc.next_log = min(rtc.next_log, now + cfg.log_s);
  if (rtc.next_push != UINT32_MAX) rtc.next_push = min(rtc.next_push, now + pushPeriod());
  // Les paramètres radio (canal, pair, clés) prennent effet au prochain démarrage de la radio.
  if (espnow::ready()) {
    espnow::end();
    if (cfg.en_espnow) espnow::begin(true);
  }
  if (!cfg.en_ble) bthome::stop();
  else if (!bthome::running()) bthome::start();
}

void setTime(int64_t epoch, const char *source) { nodeclock::set(epoch, source); }

void simReset(float soc) { ::simReset(soc); }

String statusJson() {
  uint32_t now = nodeclock::now();
  String o;
  o.reserve(1400);
  char b[160];
  auto add = [&o](const char *s) { o += s; };
  add("{");
  snprintf(b, sizeof b, "\"fw\":\"%s\",\"board\":\"%s\",\"name\":\"%s\",", VIGIE_FW_VERSION, board::kName, cfg.name);
  add(b);
  snprintf(b, sizeof b, "\"now\":%lu,\"time_valid\":%s,\"uptime\":%lu,", (unsigned long)now,
           nodeclock::valid() ? "true" : "false", (unsigned long)(now - rtc.boot_t));
  add(b);
  snprintf(b, sizeof b, "\"sensor\":\"%s\",\"sensor_ok\":%s,\"sim\":%s,\"scenario\":\"%s\",\"sim_scn\":%u,", sensorName(),
           rtc.sensor_ok ? "true" : "false", rtc.sim ? "true" : "false", simScenarioName(cfg.sim_scn), cfg.sim_scn);
  add(b);
  snprintf(b, sizeof b, "\"v\":%.3f,\"i\":%.3f,\"p\":%.2f,\"soc\":%.2f,\"soc_valid\":%s,", rtc.v, rtc.i, rtc.v * rtc.i,
           rtc.soc, rtc.soc_valid && !rtc.soc_est ? "true" : "false");
  add(b);
  if (rtc.t_ok) {
    snprintf(b, sizeof b, "\"t\":%.2f,", rtc.t);
    add(b);
  }
  snprintf(b, sizeof b, "\"ah_in\":%.3f,\"ah_out\":%.3f,\"q\":%.1f,\"ahd_in\":%.3f,\"ahd_out\":%.3f,", rtc.ah_in,
           rtc.ah_out, rtc.q_C, rtc.day.mah_in / 1000, rtc.day.mah_out / 1000);
  add(b);
  snprintf(b, sizeof b, "\"mode\":%u,\"power_state\":%u,\"alerts\":%lu,\"boots\":%lu,\"wakes\":%lu,", cfg.mode,
           rtc.power_state, (unsigned long)rtc.alert_active, (unsigned long)rtc.cold_boots, (unsigned long)rtc.wakes);
  add(b);
  snprintf(b, sizeof b, "\"rec_first\":%lu,\"rec_next\":%lu,\"rec_cap\":%lu,\"days\":%lu,\"acked\":%lu,",
           (unsigned long)flashlog::samples.firstSeq(), (unsigned long)flashlog::samples.nextSeq(),
           (unsigned long)flashlog::samples.capacity(), (unsigned long)flashlog::days.count(),
           (unsigned long)rtc.acked_rec);
  add(b);
  snprintf(b, sizeof b, "\"next_sample\":%ld,\"next_log\":%ld,\"next_push\":%ld,", (long)(rtc.next_sample - now),
           (long)(rtc.next_log - now), rtc.next_push == UINT32_MAX ? -1L : (long)(rtc.next_push - now));
  add(b);
  snprintf(b, sizeof b, "\"ble\":%s,\"ble_adv\":%lu,\"hci_fail\":%lu,", bthome::running() ? "true" : "false",
           (unsigned long)bthome::adverts, (unsigned long)bthome::hci_fail);
  add(b);
  snprintf(b, sizeof b, "\"pushes\":%lu,\"en_ok\":%lu,\"en_fail\":%lu,\"last_ack\":%ld,\"rssi\":%d,",
           (unsigned long)rtc.pushes, (unsigned long)espnow::frames_ok, (unsigned long)espnow::frames_fail,
           rtc.last_ack ? (long)(now - rtc.last_ack) : -1L, rtc.last_rssi);
  add(b);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(b, sizeof b, "\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"admin_left\":%lu,\"heap\":%lu}", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5], (unsigned long)admin::remaining(), (unsigned long)ESP.getFreeHeap());
  add(b);
  return o;
}

}  // namespace app
