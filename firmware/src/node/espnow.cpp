#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "cbor.h"
#include "config.h"
#include "flashlog.h"
#include "radio.h"
#include "record.h"
#include "state.h"
#include "tbus.h"

namespace espnow {

uint32_t frames_ok = 0, frames_fail = 0;

static bool up = false;
static bool we_started_wifi = false;
static uint8_t peer[6];
static bool broadcast = true;
static SemaphoreHandle_t sent_sem;
static volatile bool sent_ok;
static QueueHandle_t rx_q;

struct Rx {
  uint8_t len;
  int8_t rssi;
  uint8_t src[6];
  uint8_t data[tbus::kMaxFrame];
};

static void onSent(const esp_now_send_info_t *, esp_now_send_status_t status) {
  sent_ok = status == ESP_NOW_SEND_SUCCESS;
  xSemaphoreGive(sent_sem);
}

static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < tbus::kHeaderLen || len > tbus::kMaxFrame) return;
  Rx m;
  m.len = len;
  m.rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  memcpy(m.src, info->src_addr, 6);
  memcpy(m.data, data, len);
  xQueueSend(rx_q, &m, 0);
}

bool ready() { return up; }

bool begin(bool ap_active) {
  if (up) return true;
  if (!sent_sem) sent_sem = xSemaphoreCreateBinary();
  if (!rx_q) rx_q = xQueueCreate(4, sizeof(Rx));

  if (!ap_active) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    we_started_wifi = true;
    esp_wifi_set_channel(cfg.en_channel, WIFI_SECOND_CHAN_NONE);
  }
  if (cfg.en_lr) esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
  WiFi.setSleep(false);  // radio à l'écoute pour l'accusé

  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onRecv);

  uint8_t pmk[16];
  if (parseHexKey(cfg.en_pmk, pmk)) esp_now_set_pmk(pmk);

  parseMac(cfg.en_peer, peer);
  static const uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  broadcast = !memcmp(peer, bcast, 6);
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, peer, 6);
  p.channel = 0;  // canal courant
  p.ifidx = WIFI_IF_STA;
  if (!broadcast && parseHexKey(cfg.en_lmk, p.lmk)) p.encrypt = true;
  if (esp_now_add_peer(&p) != ESP_OK) {
    esp_now_deinit();
    return false;
  }
  up = true;
  return true;
}

void end() {
  if (!up) return;
  esp_now_deinit();
  up = false;
  if (we_started_wifi) {
    WiFi.mode(WIFI_OFF);
    we_started_wifi = false;
  }
}

static uint32_t uptime() { return nodeclock::now() - rtc.boot_t; }

bool send(uint8_t type, const uint8_t *payload, size_t n) {
  if (!up || n > tbus::kMaxPayload) return false;
  uint8_t frame[tbus::kMaxFrame];
  tbus::Header h{tbus::kMagic, tbus::kSchema, type, cfg.node_id, ++rtc.tb_seq, uptime()};
  memcpy(frame, &h, sizeof h);
  memcpy(frame + sizeof h, payload, n);
  xSemaphoreTake(sent_sem, 0);
  if (esp_now_send(peer, frame, sizeof h + n) != ESP_OK) {
    frames_fail++;
    return false;
  }
  bool ok = xSemaphoreTake(sent_sem, pdMS_TO_TICKS(50)) == pdTRUE && sent_ok;
  ok ? frames_ok++ : frames_fail++;
  return ok;
}

uint16_t energyFlags() {
  uint16_t f = tbus::F_MPPT_MUTE;  // pas de liaison MPPT sur Vigie 1.0
  if (rtc.i > cfg.i_tail) f |= tbus::F_CHARGING;
  if (rtc.v < cfg.v_low) f |= tbus::F_UNDERVOLT;
  if (rtc.sim) f |= tbus::F_SIMULATED;
  if (!nodeclock::valid()) f |= tbus::F_TIME_UNSYNC;
  if (!rtc.sensor_ok) f |= tbus::F_SENSOR_ERR;
  if (rtc.power_state == 2) f |= tbus::F_SURVIVAL;
  if (rtc.power_state == 3) f |= tbus::F_CUTOFF;
  return f;
}

void sendHello() {
  uint8_t b[tbus::kMaxPayload];
  CborWriter w(b, sizeof b);
  w.mapBegin();
  w.kv("id", cfg.node_id);
  w.kvs("type", "node-energy");
  w.kvs("name", cfg.name);
  w.kvs("fw", "vigie/" VIGIE_FW_VERSION);
  w.kv("caps", 0x07);  // ENERGY | LOG_BATCH | ALERT
  w.kv("wake", cfg.sample_s);
  w.kv("push", cfg.push_s);
  w.kv("rec", flashlog::samples.nextSeq() - 1);
  w.mapEnd();
  send(tbus::HELLO, b, w.size());
}

void sendEnergy(const Reading &r) {
  uint8_t b[tbus::kMaxPayload];
  CborWriter w(b, sizeof b);
  w.mapBegin();
  w.kv("ts", nodeclock::now());
  if (r.ok) {
    w.kvf("v", r.v);
    w.kvf("i", r.i);
    w.kvf("p", r.v * r.i);
  }
  w.kvf("soc", rtc.soc);
  w.kvd("q", rtc.q_C);
  if (r.t_ok) w.kvf("t_bat", r.t);
  w.kv("mode", cfg.mode);
  w.kv("flags", energyFlags());
  // Ajouts Vigie (additifs)
  w.kvf("ah_in", rtc.ah_in);
  w.kvf("ah_out", rtc.ah_out);
  w.kvf("ahd_in", rtc.day.mah_in / 1000);
  w.kvf("ahd_out", rtc.day.mah_out / 1000);
  if (rtc.day.samples) {
    w.kvf("vmin_d", rtc.day.v_min);
    w.kvf("vmax_d", rtc.day.v_max);
  }
  w.kv("rec", flashlog::samples.nextSeq() - 1);
  w.mapEnd();
  send(tbus::ENERGY, b, w.size());
}

void sendAlert(uint16_t code, uint8_t sev, float value, float limit) {
  uint8_t b[tbus::kMaxPayload];
  CborWriter w(b, sizeof b);
  w.mapBegin();
  w.kv("ts", nodeclock::now());
  w.kvs("code", vlog::alertName(code));
  w.kv("sev", sev);
  w.kvf("value", value);
  w.kvf("limit", limit);
  w.kv("flags", energyFlags());
  w.mapEnd();
  send(tbus::ALERT, b, w.size());
}

int sendBacklog() {
  // Sans récepteur connu (unicast, ou passerelle ayant déjà accusé), inutile
  // de rejouer l'historique dans le vide.
  if (broadcast && !rtc.last_ack) return 0;
  constexpr int kPerFrame = 6;  // 6 × 32 = 192 octets + ~30 d'enveloppe CBOR
  uint32_t from = max(rtc.acked_rec + 1, flashlog::samples.firstSeq());
  uint32_t end = flashlog::samples.nextSeq();
  int frames = 0;
  while (from < end && frames < cfg.en_backlog) {
    uint8_t recs[kPerFrame * vlog::kRecordSize];
    int n = 0;
    uint32_t first = from;
    while (n < kPerFrame && from < end) {
      if (flashlog::samples.read(from, recs + n * vlog::kRecordSize)) n++;
      from++;
    }
    if (!n) continue;
    uint8_t b[tbus::kMaxPayload];
    CborWriter w(b, sizeof b);
    w.mapBegin();
    w.kv("first", first);
    w.kv("last", from - 1);
    w.kv("sz", vlog::kRecordSize);
    w.text("r");
    w.bytes(recs, n * vlog::kRecordSize);
    w.mapEnd();
    if (!send(tbus::LOG_BATCH, b, w.size())) break;
    frames++;
    poll(cfg.en_wait_ms);  // l'accusé fait avancer acked_rec
    // Accusé perdu : on s'arrête là, le lot sera rejoué au prochain push. Le
    // récepteur n'accuse ainsi jamais au-delà d'un trou.
    if (rtc.acked_rec < from - 1) break;
  }
  return frames;
}

static void handle(const Rx &m) {
  tbus::Header h;
  memcpy(&h, m.data, sizeof h);
  if (h.magic != tbus::kMagic) return;
  const uint8_t *p = m.data + sizeof h;
  size_t n = m.len - sizeof h;
  CborReader rd(p, n);
  double v;
  switch (h.msg_type) {
    case tbus::ACK:
      rtc.last_ack = nodeclock::now();
      rtc.last_rssi = m.rssi;
      if (rd.findNumber("rec", v) && v > rtc.acked_rec && v < flashlog::samples.nextSeq()) rtc.acked_rec = (uint32_t)v;
      [[fallthrough]];
    case tbus::WELCOME:
      if (rd.findNumber("utc", v) && v > nodeclock::kValidAfter) {
        int64_t diff = (int64_t)v - (int64_t)nodeclock::now();
        if (!nodeclock::valid() || diff > 2 || diff < -2) nodeclock::set((int64_t)v, "espnow");
      }
      if (h.msg_type == tbus::WELCOME && rd.findNumber("mode", v) && v >= 0 && v <= 3) cfg.mode = (uint8_t)v;
      break;
  }
}

void poll(uint32_t wait_ms) {
  if (!rx_q) return;
  uint32_t t0 = millis();
  Rx m;
  do {
    uint32_t left = wait_ms - min(wait_ms, (uint32_t)(millis() - t0));
    if (xQueueReceive(rx_q, &m, pdMS_TO_TICKS(left)) == pdTRUE) handle(m);
    else break;
  } while (millis() - t0 < wait_ms);
  while (xQueueReceive(rx_q, &m, 0) == pdTRUE) handle(m);
}

}  // namespace espnow
