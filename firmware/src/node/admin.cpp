// Administration : point d'accès Wi-Fi, portail captif et page web, ouverts
// pendant cfg.ap_window_s après un démarrage à froid ou un appui sur BOOT.
// Chaque requête HTTP repousse la fermeture de 2 min (la page interroge
// l'état toutes les 5 s : page ouverte = nœud éveillé), dans la limite d'une
// heure par ouverture pour ne pas vider la batterie si un téléphone reste
// connecté.
#include <DNSServer.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <time.h>

#include "app.h"
#include "config.h"
#include "flashlog.h"
#include "record.h"
#include "sensor.h"
#include "state.h"
#include "web_ui.h"

namespace admin {

static WebServer server(80);
static DNSServer dns;
static bool running = false;
static uint32_t opened_ms, until_ms;
constexpr uint32_t kMaxOpenMs = 3600 * 1000;
static String ota_error;

bool active() { return running && (int32_t)(until_ms - millis()) > 0; }
uint32_t remaining() { return active() ? (until_ms - millis()) / 1000 : 0; }

void extend(uint32_t s) {
  uint32_t t = millis() + s * 1000;
  uint32_t cap = opened_ms + kMaxOpenMs;
  if ((int32_t)(t - cap) > 0) t = cap;
  if ((int32_t)(t - until_ms) > 0) until_ms = t;
}

void close() { until_ms = millis(); }

static void touch() { extend(120); }

static void sendJson(const String &s, int code = 200) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", s);
}

static void handleConfigPost() {
  touch();
  Config backup = cfg;
  String errors;
  for (int k = 0; k < server.args(); k++) {
    String key = server.argName(k);
    const Field *f = configFind(key.c_str());
    if (!f) continue;
    String v = server.arg(k);
    if (f->secret && v.isEmpty()) continue;  // champ secret laissé vide : inchangé
    if (f->secret && v == "-") v = "";       // « - » efface la clé
    if (!configSet(key.c_str(), v.c_str())) {
      if (errors.length()) errors += ',';
      errors += '"' + key + '"';
    }
  }
  if (errors.length()) {
    cfg = backup;
    sendJson("{\"ok\":false,\"errors\":[" + errors + "]}", 400);
    return;
  }
  if (!configSave()) {
    sendJson("{\"ok\":false,\"errors\":[\"nvs\"]}", 500);
    return;
  }
  app::configChanged();
  sendJson("{\"ok\":true}");
}

static void handleAction() {
  touch();
  String what = server.arg("do");
  if (what == "push") app::pushNow();
  else if (what == "sample") app::sampleNow();
  else if (what == "extend") {
    opened_ms = millis();
    extend(server.hasArg("s") ? server.arg("s").toInt() : 900);
  } else if (what == "sleep") {
    sendJson("{\"ok\":true}");
    delay(200);
    app::sleepNow();
    return;
  } else if (what == "reboot") {
    sendJson("{\"ok\":true}");
    delay(300);
    ESP.restart();
  } else if (what == "soc") {
    float v = server.arg("value").toFloat();
    if (v < 0 || v > 100) return sendJson("{\"ok\":false}", 400);
    app::setSoc(v);
  } else if (what == "time") {
    int64_t e = atoll(server.arg("epoch").c_str());
    if (e < nodeclock::kValidAfter) return sendJson("{\"ok\":false}", 400);
    int64_t d = e - (int64_t)nodeclock::now();
    if (!nodeclock::valid() || d > 2 || d < -2) app::setTime(e, "navigateur");
  } else if (what == "sim") {
    if (server.hasArg("scn")) {
      configSet("sim_scn", server.arg("scn").c_str());
      configSave();
    }
    if (server.hasArg("soc")) app::simReset(server.arg("soc").toFloat());
    app::sampleNow();
  } else if (what == "erase") {
    if (server.arg("confirm") != "EFFACER") return sendJson("{\"ok\":false}", 400);
    app::eraseLogs();
  } else {
    return sendJson("{\"ok\":false}", 400);
  }
  sendJson("{\"ok\":true}");
}

static void isoTime(char *out, size_t n, uint32_t ts, bool valid) {
  if (!valid) {
    snprintf(out, n, "+%lus", (unsigned long)ts);
    return;
  }
  time_t t = ts;
  struct tm tm;
  gmtime_r(&t, &tm);
  strftime(out, n, "%Y-%m-%dT%H:%M:%SZ", &tm);
}

static const char *typeName(uint8_t t) {
  static const char *n[] = {"?", "sample", "boot", "time_sync", "soc_set", "alert", "config", "cutoff"};
  return t < 8 ? n[t] : "?";
}

static void beginStream(const char *type, const char *filename) {
  server.sendHeader("Content-Disposition", String("attachment; filename=\"") + filename + "\"");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, type, "");
}

static void handleLogCsv() {
  touch();
  char fname[48];
  snprintf(fname, sizeof fname, "vigie-%s-log.csv", cfg.name);
  beginStream("text/csv", fname);
  server.sendContent(
      "seq,type,ts,utc,time_valid,sim,mode,flags,v_avg,v_min,v_max,i_avg,i_min,i_max,soc,t,dt_s\n");
  String chunk;
  chunk.reserve(4200);
  vlog::Record r;
  char line[200], iso[24];
  for (uint32_t s = flashlog::samples.firstSeq(); s < flashlog::samples.nextSeq(); s++) {
    if (!flashlog::samples.read(s, &r)) continue;
    bool tv = r.flags & vlog::RF_TIME_VALID;
    isoTime(iso, sizeof iso, r.ts, tv);
    char t[12] = "";
    if (r.t_cC != vlog::kNoTemp) snprintf(t, sizeof t, "%.2f", r.t_cC / 100.0);
    snprintf(line, sizeof line, "%lu,%s,%lu,%s,%d,%d,%d,0x%02X,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%s,%u\n",
             (unsigned long)r.seq, typeName(r.type_mode & 0x0F), (unsigned long)r.ts, iso, tv ? 1 : 0,
             (r.flags & vlog::RF_SIM) ? 1 : 0, (r.type_mode >> 4) & 3, r.flags, r.v_avg_mV / 1000.0,
             r.v_min_mV / 1000.0, r.v_max_mV / 1000.0, r.i_avg_mA / 1000.0, r.i_min_cA / 100.0, r.i_max_cA / 100.0,
             r.soc_cp / 100.0, t, r.dt_s);
    chunk += line;
    if (chunk.length() > 4000) {
      server.sendContent(chunk);
      chunk = "";
    }
  }
  if (chunk.length()) server.sendContent(chunk);
  server.sendContent("");
}

static void handleDaysCsv() {
  touch();
  char fname[48];
  snprintf(fname, sizeof fname, "vigie-%s-jours.csv", cfg.name);
  beginStream("text/csv", fname);
  server.sendContent("seq,utc,time_valid,hours,ah_in,ah_out,v_min,v_max,soc_min,soc_max,t_min,t_max,sim\n");
  vlog::DayRecord r;
  char line[200], iso[24];
  for (uint32_t s = flashlog::days.firstSeq(); s < flashlog::days.nextSeq(); s++) {
    if (!flashlog::days.read(s, &r)) continue;
    bool tv = r.flags & vlog::RF_TIME_VALID;
    isoTime(iso, sizeof iso, r.ts, tv);
    snprintf(line, sizeof line, "%lu,%s,%d,%u,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f,%d\n", (unsigned long)r.seq, iso,
             tv ? 1 : 0, r.hours, r.mah_in / 1000.0, r.mah_out / 1000.0, r.v_min_mV / 1000.0, r.v_max_mV / 1000.0,
             r.soc_min_cp / 100.0, r.soc_max_cp / 100.0, r.t_min_cC / 100.0, r.t_max_cC / 100.0,
             (r.flags & vlog::RF_SIM) ? 1 : 0);
    server.sendContent(line);
  }
  server.sendContent("");
}

static void handleLogBin() {
  touch();
  char fname[48];
  snprintf(fname, sizeof fname, "vigie-%s-log.bin", cfg.name);
  beginStream("application/octet-stream", fname);
  uint8_t buf[32 * vlog::kRecordSize];
  size_t n = 0;
  for (uint32_t s = flashlog::samples.firstSeq(); s < flashlog::samples.nextSeq(); s++) {
    if (!flashlog::samples.read(s, buf + n)) continue;
    n += vlog::kRecordSize;
    if (n == sizeof buf) {
      server.sendContent((const char *)buf, n);
      n = 0;
    }
  }
  if (n) server.sendContent((const char *)buf, n);
  server.sendContent("");
}

// Série pour le graphique : échantillons des `h` dernières heures, ≤ 600 points.
static void handleSeries() {
  touch();
  int h = constrain(server.arg("h").toInt(), 1, 24 * 90);
  uint32_t now = nodeclock::now();
  uint32_t since = now > (uint32_t)h * 3600 ? now - h * 3600 : 0;
  uint32_t first = flashlog::samples.firstSeq(), next = flashlog::samples.nextSeq();
  // Remonte depuis la fin jusqu'à la borne de temps.
  uint32_t start = next;
  vlog::Record r;
  while (start > first) {
    if (flashlog::samples.read(start - 1, &r) && (r.type_mode & 0x0F) == vlog::REC_SAMPLE && r.ts < since) break;
    start--;
  }
  uint32_t span = next - start;
  uint32_t stride = span / 600 + 1;
  String t = "[", v = "[", i = "[", s = "[", tv = "[";
  bool firstPt = true;
  char b[24];
  for (uint32_t q = start; q < next; q += stride) {
    if (!flashlog::samples.read(q, &r) || (r.type_mode & 0x0F) != vlog::REC_SAMPLE) continue;
    if (!firstPt) { t += ','; v += ','; i += ','; s += ','; }
    firstPt = false;
    t += String(r.ts + r.dt_s / 2);
    snprintf(b, sizeof b, "%.3f", r.v_avg_mV / 1000.0); v += b;
    snprintf(b, sizeof b, "%.3f", r.i_avg_mA / 1000.0); i += b;
    snprintf(b, sizeof b, "%.2f", r.soc_cp / 100.0); s += b;
  }
  sendJson("{\"t\":" + t + "],\"v\":" + v + "],\"i\":" + i + "],\"soc\":" + s + "]}");
}

static void handleOtaDone() {
  touch();
  bool ok = !Update.hasError() && ota_error.isEmpty();
  sendJson(ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"" + ota_error + "\"}", ok ? 200 : 500);
  if (ok) {
    delay(500);
    ESP.restart();
  }
}

static void handleOtaUpload() {
  HTTPUpload &up = server.upload();
  touch();
  if (up.status == UPLOAD_FILE_START) {
    ota_error = "";
    Serial.printf("[ota] %s\n", up.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) ota_error = Update.errorString();
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (ota_error.isEmpty() && Update.write(up.buf, up.currentSize) != up.currentSize) ota_error = Update.errorString();
  } else if (up.status == UPLOAD_FILE_END) {
    if (ota_error.isEmpty() && !Update.end(true)) ota_error = Update.errorString();
    Serial.printf("[ota] %u octets, %s\n", (unsigned)up.totalSize, ota_error.isEmpty() ? "OK" : ota_error.c_str());
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    ota_error = "interrompu";
  }
}

static void captive() {
  touch();
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

void start() {
  char ssid[33];
  if (cfg.ap_ssid[0]) {
    strlcpy(ssid, cfg.ap_ssid, sizeof ssid);
  } else {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);  // WiFi.macAddress() rend des zéros avant l'init
    snprintf(ssid, sizeof ssid, "Vigie-%02X%02X", mac[4], mac[5]);
  }
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);  // STA : interface d'ESP-NOW, même canal que le point d'accès
  WiFi.softAP(ssid, cfg.ap_pass, cfg.en_channel, 0, 4);
  esp_wifi_set_channel(cfg.en_channel, WIFI_SECOND_CHAN_NONE);
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, [] {
    touch();
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8", kWebUi);
  });
  server.on("/api/status", HTTP_GET, [] { touch(); sendJson(app::statusJson()); });
  server.on("/api/config", HTTP_GET, [] { touch(); sendJson(configJson()); });
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/action", HTTP_POST, handleAction);
  server.on("/api/series", HTTP_GET, handleSeries);
  server.on("/log.csv", HTTP_GET, handleLogCsv);
  server.on("/days.csv", HTTP_GET, handleDaysCsv);
  server.on("/log.bin", HTTP_GET, handleLogBin);
  server.on("/update", HTTP_POST, handleOtaDone, handleOtaUpload);
  // Détection de portail captif (Android, Apple, Windows) : on redirige tout.
  server.onNotFound(captive);
  server.begin();

  running = true;
  opened_ms = millis();
  until_ms = opened_ms + cfg.ap_window_s * 1000;
  Serial.printf("[admin] point d'accès « %s » canal %u, http://%s/ pendant %u s%s\n", ssid, cfg.en_channel,
                WiFi.softAPIP().toString().c_str(), cfg.ap_window_s,
                strcmp(cfg.ap_pass, "vigie-admin") ? "" : " — mot de passe par défaut « vigie-admin »");
}

void stop() {
  if (!running) return;
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  running = false;
}

void loop() {
  if (!running) return;
  dns.processNextRequest();
  server.handleClient();
}

}  // namespace admin
