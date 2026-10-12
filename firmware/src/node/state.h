// État conservé en RTC RAM : survit au sommeil profond, perdu à la coupure
// d'alimentation ou au reset. Ce qui doit survivre à une coupure est
// reconstruit depuis le journal flash (dernier SOC) ou la NVS (compteurs).
#pragma once
#include <stdint.h>
#include <sys/time.h>

// Horloge du nœud. Avant tout recalage, elle compte depuis la mise sous
// tension (epoch 0) ; les enregistrements portent alors RF_TIME_VALID = 0.
namespace nodeclock {
constexpr int64_t kValidAfter = 1700000000;  // 2023-11 : toute date antérieure = non recalée
inline int64_t nowUs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}
inline uint32_t now() { return (uint32_t)(nowUs() / 1000000); }
inline bool valid() { return nowUs() / 1000000 >= kValidAfter; }
// Recale l'horloge. Rend le décalage appliqué (s). Déplace aussi toutes les
// échéances de l'état RTC pour que les cadences ne sautent pas.
int64_t set(int64_t epoch, const char *source);
}  // namespace nodeclock

struct SimState {
  uint32_t rng;
  double soc;          // SOC « vrai » du modèle, %
  uint32_t last_t;     // horloge nœud du dernier pas de simulation
  float cloud;         // 0,2 … 1
  uint32_t cloud_t;
  uint32_t full_since; // pour la bascule absorption → floating
  uint8_t day_evening; // tirage « soirée à bord » du jour
  uint16_t day_id;
};

struct Agg {  // agrégat de la période d'enregistrement en cours
  uint32_t start;
  uint16_t n;
  double v_sum, i_sum;
  float v_min, v_max, i_min, i_max;
  float t_sum;
  uint16_t t_n;
  uint8_t flags;
};

struct DayAgg {
  uint32_t start;
  double mah_in, mah_out;
  float v_min, v_max, soc_min, soc_max, t_min, t_max;
  uint32_t samples;
  uint8_t flags;
};

struct RtcState {
  uint32_t magic;
  uint32_t wakes;           // réveils depuis la mise sous tension
  uint32_t cold_boots;      // copie du compteur NVS
  uint32_t boot_t;          // horloge nœud à la mise sous tension (décalée avec elle)

  // Échéances, horloge nœud (s)
  uint32_t next_sample, next_log, next_push, next_day;

  // Dernière mesure
  float v, i, t;
  bool t_ok, sensor_ok, sim;
  uint32_t last_sample;     // horloge nœud (s)
  int64_t last_sample_us;

  // Estimateur de SOC
  double soc;               // %
  double q_C;               // charge nette cumulée depuis la mise en service (C)
  double ah_in, ah_out;     // cumuls de vie (Ah), persistés en NVS
  uint32_t full_since;      // début de la condition « pleine », 0 = non remplie
  uint32_t rest_since;      // début du repos (|I| faible)
  bool rest_done;           // recalage OCV déjà fait pour ce repos
  uint32_t last_charge;     // dernière charge nette observée (> i_tail)
  bool soc_valid;           // SOC initialisé
  bool soc_est;             // estimé sur la tension sous charge, pas encore recalé

  Agg agg;
  DayAgg day;

  // Protection et alertes
  uint32_t alert_active;    // bits (1 << AlertCode)
  uint32_t alert_pending;   // levées, pas encore poussées
  uint32_t alert_last[16];  // dernière émission par code
  uint8_t power_state;      // 0 nominal, 1 réduit, 2 survie, 3 coupure
  uint8_t low_count;        // évaluations consécutives sous le seuil (anti-démarreur)
  uint8_t logs_since_persist;
  bool hello_pending;

  // Radio
  uint16_t tb_seq;
  uint32_t ble_counter;
  uint8_t ble_pkt;
  uint32_t acked_rec;       // dernier enregistrement accusé par un récepteur
  uint32_t last_ack;        // horloge nœud du dernier accusé reçu
  uint32_t pushes, push_fail;
  int8_t last_rssi;

  // Administration
  bool admin_requested;

  SimState sim_state;
};

extern RtcState rtc;
constexpr uint32_t kRtcMagic = 0x56494731;  // « VIG1 »
