// Annonce BLE au format BTHome v2 (https://bthome.io) — lue telle quelle par
// Home Assistant, Theengs, ESPHome proxy, et tools/vigie_ble.py.
//
// Pas de pile Bluetooth hôte : on parle directement au contrôleur en HCI via
// VHCI. Pour une simple balise, c'est plus rapide à démarrer (compte pour
// chaque réveil) et bien plus léger que Bluedroid.
#include <Arduino.h>
#include <Preferences.h>
#include <esp_bt.h>
#include <esp_mac.h>
#include <freertos/semphr.h>
#include <mbedtls/ccm.h>

#include "config.h"
#include "radio.h"
#include "state.h"

// Garder la mémoire du contrôleur BLE (Arduino la libère sinon au démarrage).
extern "C" bool bleInUse(void) { return true; }

namespace bthome {

static SemaphoreHandle_t cmd_sem;
static volatile bool started = false;
static bool vhci_registered = false;
static uint32_t ctr_reserved = 0;
uint32_t hci_ok = 0, hci_fail = 0, adverts = 0;  // fin de la plage de compteurs réservée en NVS

static void onSendAvailable() {}
static int onRecv(uint8_t *data, uint16_t len) {
  // Événement HCI : 0x04, code 0x0E (Command Complete) ou 0x0F (Command Status)
  if (len >= 2 && data[0] == 0x04 && (data[1] == 0x0E || data[1] == 0x0F)) xSemaphoreGive(cmd_sem);
  return 0;
}
static const esp_vhci_host_callback_t kCallbacks = {onSendAvailable, onRecv};

static bool hci(uint16_t opcode, const uint8_t *params, uint8_t n) {
  uint8_t pkt[4 + 32];
  pkt[0] = 0x01;  // commande HCI
  pkt[1] = opcode & 0xFF;
  pkt[2] = opcode >> 8;
  pkt[3] = n;
  memcpy(pkt + 4, params, n);
  uint32_t t0 = millis();
  while (!esp_vhci_host_check_send_available()) {
    if (millis() - t0 > 100) return false;
    delay(1);
  }
  xSemaphoreTake(cmd_sem, 0);
  esp_vhci_host_send_packet(pkt, 4 + n);
  if (xSemaphoreTake(cmd_sem, pdMS_TO_TICKS(200)) == pdTRUE) {
    hci_ok++;
    return true;
  }
  hci_fail++;
  Serial.printf("[ble] commande HCI 0x%04X sans réponse\n", opcode);
  return false;
}

constexpr uint16_t OP_RESET = 0x0C03, OP_ADV_PARAMS = 0x2006, OP_ADV_DATA = 0x2008, OP_SCAN_RSP = 0x2009,
                   OP_ADV_ENABLE = 0x200A;

void initCounter() {
  // Le compteur de chiffrement doit croître d'une annonce à l'autre, même à
  // travers une coupure : on réserve des plages de 100 000 en NVS.
  Preferences p;
  if (!p.begin("ble", false)) return;
  uint32_t base = p.getUInt("ctr", 1);
  p.putUInt("ctr", base + 100000);
  p.end();
  rtc.ble_counter = base;
  ctr_reserved = base + 100000;
}

static uint32_t nextCounter() {
  uint32_t c = ++rtc.ble_counter;
  if (ctr_reserved == 0 || c >= ctr_reserved) {  // réveil : la plage est en NVS
    Preferences p;
    if (p.begin("ble", false)) {
      uint32_t base = p.getUInt("ctr", 1);
      if (c >= base) {
        p.putUInt("ctr", c + 100000);
        ctr_reserved = c + 100000;
      } else {
        ctr_reserved = base;
      }
      p.end();
    }
  }
  return c;
}

bool running() { return started; }

// Réponse au scan (seuls les scanners actifs la demandent, dont iOS) :
//  - données constructeur 0xFFFF (identifiant réservé aux essais) :
//    'V', version 1, MAC Bluetooth (iOS la masque, or le déchiffrement BTHome
//    en a besoin pour le nonce), drapeaux d'état tbus::EnergyFlags, alertes
//    actives (bit n = AlertCode n). En clair, même si l'annonce est chiffrée.
//  - le nom, tronqué pour tenir dans les 31 octets.
static void scanResponse() {
  uint8_t rsp[32] = {0};
  uint8_t *p = rsp + 1;
  *p++ = 15;
  *p++ = 0xFF;
  *p++ = 0xFF; *p++ = 0xFF;
  *p++ = 'V'; *p++ = 1;
  esp_read_mac(p, ESP_MAC_BT);
  p += 6;
  uint16_t flags = espnow::energyFlags();
  uint16_t alerts = rtc.alert_active & 0xFFFF;
  *p++ = flags; *p++ = flags >> 8;
  *p++ = alerts; *p++ = alerts >> 8;
  char name[32];
  snprintf(name, sizeof name, "Vigie %s", cfg.name);
  size_t room = 31 - (p - rsp - 1) - 2;
  size_t nl = min(strlen(name), room);
  *p++ = 1 + nl;
  *p++ = nl < strlen(name) ? 0x08 : 0x09;  // nom abrégé / complet
  memcpy(p, name, nl);
  p += nl;
  rsp[0] = p - rsp - 1;
  hci(OP_SCAN_RSP, rsp, 32);
}

bool start() {
  if (started) return true;
  if (!cmd_sem) cmd_sem = xSemaphoreCreateBinary();
  if (!btStarted() && !btStartMode(BT_MODE_BLE)) {
    Serial.println("[ble] contrôleur indisponible");
    return false;
  }
  if (!vhci_registered) {
    esp_vhci_host_register_callback(&kCallbacks);
    vhci_registered = true;
  }
  if (!hci(OP_RESET, nullptr, 0)) return false;

  // Annonce scannable non connectable, 100 ms, sur les trois canaux.
  uint8_t params[15] = {0xA0, 0x00, 0xA0, 0x00, 0x02 /* ADV_SCAN_IND */, 0x00 /* adresse publique */, 0, 0, 0, 0, 0, 0, 0, 0x07, 0x00};
  if (!hci(OP_ADV_PARAMS, params, sizeof params)) return false;

  started = true;
  return true;
}

static void put16(uint8_t *&p, uint16_t v) { *p++ = v; *p++ = v >> 8; }
static void put32(uint8_t *&p, uint32_t v) { for (int i = 0; i < 4; i++) *p++ = v >> (8 * i); }

void update(const Reading &r, float soc) {
  if (!started) return;
  uint8_t key[16];
  bool enc = parseHexKey(cfg.ble_key, key);

  // Objets BTHome, par identifiant croissant (exigé par la spécification).
  uint8_t obj[24], *p = obj;
  if (!enc) { *p++ = 0x00; *p++ = ++rtc.ble_pkt; }                        // packet id
  *p++ = 0x01; *p++ = (uint8_t)lroundf(fminf(100, fmaxf(0, soc)));         // batterie %
  if (r.t_ok) { *p++ = 0x02; put16(p, (int16_t)lroundf(r.t * 100)); }       // température 0,01 °C
  if (r.ok) {
    *p++ = 0x0C; put16(p, (uint16_t)lroundf(r.v * 1000));                    // tension 1 mV
    if (!enc) { *p++ = 0x5C; put32(p, (uint32_t)(int32_t)lroundf(r.v * r.i * 100)); }  // puissance signée 0,01 W
    float ma = fminf(32767, fmaxf(-32768, r.i * 1000));                     // courant signé 1 mA (±32 A)
    *p++ = 0x5D; put16(p, (uint16_t)(int16_t)lroundf(ma));
  }
  size_t objlen = p - obj;

  uint8_t adv[32] = {0};
  uint8_t *a = adv + 1;
  *a++ = 0x02; *a++ = 0x01; *a++ = 0x06;  // Flags : LE General Discoverable, pas de BR/EDR
  uint8_t *svc_len = a++;
  *a++ = 0x16; *a++ = 0xD2; *a++ = 0xFC;  // Service Data, UUID 0xFCD2
  uint8_t devinfo = 0x40 | (enc ? 0x01 : 0x00);
  *a++ = devinfo;
  if (enc) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    uint32_t ctr = nextCounter();
    uint8_t nonce[13];
    memcpy(nonce, mac, 6);
    nonce[6] = 0xD2;
    nonce[7] = 0xFC;
    nonce[8] = devinfo;
    memcpy(nonce + 9, &ctr, 4);
    mbedtls_ccm_context ccm;
    mbedtls_ccm_init(&ccm);
    mbedtls_ccm_setkey(&ccm, MBEDTLS_CIPHER_ID_AES, key, 128);
    uint8_t tag[4];
    mbedtls_ccm_encrypt_and_tag(&ccm, objlen, nonce, 13, nullptr, 0, obj, a, tag, 4);
    mbedtls_ccm_free(&ccm);
    a += objlen;
    memcpy(a, &ctr, 4);
    a += 4;
    memcpy(a, tag, 4);
    a += 4;
  } else {
    memcpy(a, obj, objlen);
    a += objlen;
  }
  *svc_len = a - svc_len - 1;
  adv[0] = a - adv - 1;
  if (adv[0] > 31) return;  // ne doit pas arriver : 27 octets au plus

  uint8_t off = 0, on = 1;
  hci(OP_ADV_ENABLE, &off, 1);
  scanResponse();
  if (hci(OP_ADV_DATA, adv, 32) && hci(OP_ADV_ENABLE, &on, 1)) adverts++;
}

void stop() {
  if (!started) return;
  uint8_t off = 0;
  hci(OP_ADV_ENABLE, &off, 1);
  btStop();
  started = false;
}

}  // namespace bthome
