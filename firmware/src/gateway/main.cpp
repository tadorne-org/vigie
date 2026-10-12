// Passerelle de banc TsubameBus : un second ESP32 branché en USB.
//
//  - écoute ESP-NOW sur le canal 1 et imprime chaque trame en une ligne JSON
//    (le corps CBOR est rendu en diagnostic lisible) ;
//  - acquitte ENERGY et LOG_BATCH (ACK 0x21 avec `rec` = dernier
//    enregistrement reçu) : le nœud avance alors son pointeur d'historique ;
//  - répond à HELLO par WELCOME, et glisse l'heure UTC dans chaque accusé si
//    on la lui a donnée sur le port série : `T<epoch>` (tools/vigie_gateway.py
//    le fait à la connexion).
//
// Préfigure le rôle de réception du module B ; pas de chiffrement
// ici : la passerelle de banc accepte tout le monde.
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <sys/time.h>

#include "cbor.h"
#include "record.h"
#include "tbus.h"

constexpr uint8_t kChannel = 1;
constexpr uint16_t kGatewayId = 0xB001;

struct Rx {
  uint8_t len;
  int8_t rssi;
  uint8_t src[6];
  uint8_t data[tbus::kMaxFrame];
};
static QueueHandle_t rx_q;
static uint16_t seq = 0;
static bool time_ok = false;

static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < tbus::kHeaderLen || len > tbus::kMaxFrame) return;
  Rx m;
  m.len = len;
  m.rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  memcpy(m.src, info->src_addr, 6);
  memcpy(m.data, data, len);
  xQueueSend(rx_q, &m, 0);
}

// Rend un élément CBOR en JSON approché (diagnostic, pas un convertisseur complet).
static size_t cborToJson(const uint8_t *p, size_t n, size_t pos, String &out, int depth = 0) {
  if (pos >= n || depth > 6) return n;
  uint8_t ib = p[pos++];
  uint8_t major = ib >> 5, ai = ib & 0x1F;
  auto arg = [&](uint64_t &v) {
    if (ai < 24) { v = ai; return; }
    int nb = ai == 24 ? 1 : ai == 25 ? 2 : ai == 26 ? 4 : 8;
    v = 0;
    for (int k = 0; k < nb && pos < n; k++) v = (v << 8) | p[pos++];
  };
  uint64_t v = 0;
  switch (major) {
    case 0: arg(v); out += String((unsigned long long)v); return pos;
    case 1: arg(v); out += String(-1LL - (long long)v); return pos;
    case 2: {
      arg(v);
      out += "\"";
      for (uint64_t k = 0; k < v && pos < n; k++) {
        char h[3];
        snprintf(h, 3, "%02x", p[pos++]);
        out += h;
      }
      out += "\"";
      return pos;
    }
    case 3: arg(v); out += '"'; for (uint64_t k = 0; k < v && pos < n; k++) out += (char)p[pos++]; out += '"'; return pos;
    case 4:
    case 5: {
      bool map = major == 5, indef = ai == 31;
      if (!indef) arg(v);
      out += map ? '{' : '[';
      for (uint64_t k = 0; indef ? (pos < n && p[pos] != 0xFF) : k < v; k++) {
        if (k) out += ',';
        pos = cborToJson(p, n, pos, out, depth + 1);
        if (map) { out += ':'; pos = cborToJson(p, n, pos, out, depth + 1); }
      }
      if (indef) pos++;
      out += map ? '}' : ']';
      return pos;
    }
    case 7:
      if (ib == 0xF4) { out += "false"; return pos; }
      if (ib == 0xF5) { out += "true"; return pos; }
      if (ib == 0xFA) {
        uint32_t u = 0;
        for (int k = 0; k < 4; k++) u = (u << 8) | p[pos++];
        float f;
        memcpy(&f, &u, 4);
        out += String(f, 4);
        return pos;
      }
      if (ib == 0xFB) {
        uint64_t u = 0;
        for (int k = 0; k < 8; k++) u = (u << 8) | p[pos++];
        double d;
        memcpy(&d, &u, 8);
        out += String(d, 3);
        return pos;
      }
  }
  out += "null";
  return n;
}

static void reply(const uint8_t *mac, uint8_t type, uint16_t acked_seq, double rec) {
  if (!esp_now_is_peer_exist(mac)) {
    esp_now_peer_info_t p = {};
    memcpy(p.peer_addr, mac, 6);
    p.ifidx = WIFI_IF_STA;
    esp_now_add_peer(&p);
  }
  uint8_t b[64];
  CborWriter w(b, sizeof b);
  w.mapBegin();
  w.kv("seq", acked_seq);
  if (rec >= 0) w.kv("rec", (uint64_t)rec);
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  if (time_ok) w.kv("utc", tv.tv_sec);
  if (type == tbus::WELCOME) w.kv("mode", 1);
  w.mapEnd();
  uint8_t frame[tbus::kMaxFrame];
  tbus::Header h{tbus::kMagic, tbus::kSchema, type, kGatewayId, ++seq, (uint32_t)(millis() / 1000)};
  memcpy(frame, &h, sizeof h);
  memcpy(frame + sizeof h, b, w.size());
  esp_now_send(mac, frame, sizeof h + w.size());
}

static const char *typeName(uint8_t t) {
  switch (t) {
    case tbus::HELLO: return "HELLO";
    case tbus::WELCOME: return "WELCOME";
    case tbus::ENERGY: return "ENERGY";
    case tbus::LOG_BATCH: return "LOG_BATCH";
    case tbus::ALERT: return "ALERT";
    case tbus::ACK: return "ACK";
  }
  return "?";
}

void setup() {
  Serial.begin(115200);
  rx_q = xQueueCreate(16, sizeof(Rx));
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(kChannel, WIFI_SECOND_CHAN_NONE);
  WiFi.setSleep(false);
  esp_now_init();
  esp_now_register_recv_cb(onRecv);
  Serial.printf("{\"gateway\":\"%s\",\"channel\":%u}\n", WiFi.macAddress().c_str(), kChannel);
}

void loop() {
  if (Serial.available()) {
    String l = Serial.readStringUntil('\n');
    if (l.startsWith("T")) {
      struct timeval tv = {(time_t)atoll(l.c_str() + 1), 0};
      settimeofday(&tv, nullptr);
      time_ok = true;
      Serial.printf("{\"time_set\":%ld}\n", (long)tv.tv_sec);
    }
  }
  Rx m;
  if (xQueueReceive(rx_q, &m, pdMS_TO_TICKS(10)) != pdTRUE) return;
  tbus::Header h;
  memcpy(&h, m.data, sizeof h);
  if (h.magic != tbus::kMagic) return;
  const uint8_t *body = m.data + sizeof h;
  size_t n = m.len - sizeof h;

  String out;
  out.reserve(600);
  char head[160];
  snprintf(head, sizeof head,
           "{\"from\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"rssi\":%d,\"src\":%u,\"type\":\"%s\",\"seq\":%u,\"up\":%lu,\"body\":",
           m.src[0], m.src[1], m.src[2], m.src[3], m.src[4], m.src[5], m.rssi, h.src_id, typeName(h.msg_type), h.seq,
           (unsigned long)h.uptime_s);
  out += head;
  cborToJson(body, n, 0, out);

  CborReader rd(body, n);
  double rec = -1;
  if (h.msg_type == tbus::LOG_BATCH) {
    rd.findNumber("last", rec);
    double first = 0;
    rd.findNumber("first", first);
    out += ",\"records\":" + String((unsigned long)(rec - first + 1));
  }
  out += "}";
  Serial.println(out);

  // Accusés : ENERGY accuse l'heure, LOG_BATCH fait avancer l'historique.
  if (h.msg_type == tbus::HELLO) reply(m.src, tbus::WELCOME, h.seq, -1);
  else if (h.msg_type == tbus::LOG_BATCH) reply(m.src, tbus::ACK, h.seq, rec);
  else if (h.msg_type == tbus::ENERGY) reply(m.src, tbus::ACK, h.seq, -1);
}
