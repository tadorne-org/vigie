#include "battery.h"

#include <math.h>

#include "config.h"
#include "state.h"

// Table usuelle plomb ouvert / AGM, repos > 4 h.
static const float kOcvV[] = {11.89f, 12.06f, 12.24f, 12.45f, 12.65f};
static const float kOcvSoc[] = {0, 25, 50, 75, 100};
constexpr int kOcvN = 5;

float ocvFromSoc(float soc) {
  if (soc <= 0) return kOcvV[0];
  for (int k = 1; k < kOcvN; k++)
    if (soc <= kOcvSoc[k])
      return kOcvV[k - 1] + (kOcvV[k] - kOcvV[k - 1]) * (soc - kOcvSoc[k - 1]) / (kOcvSoc[k] - kOcvSoc[k - 1]);
  return kOcvV[kOcvN - 1];
}

float socFromOcv(float v) {
  if (v <= kOcvV[0]) return 0;
  for (int k = 1; k < kOcvN; k++)
    if (v <= kOcvV[k])
      return kOcvSoc[k - 1] + (kOcvSoc[k] - kOcvSoc[k - 1]) * (v - kOcvV[k - 1]) / (kOcvV[k] - kOcvV[k - 1]);
  return 100;
}

void batterySetSoc(float soc) {
  rtc.soc = fminf(100, fmaxf(0, soc));
  rtc.soc_valid = true;
  rtc.soc_est = false;
}

SocEvent batteryUpdate(const Reading &r, uint32_t now, float dt_s, float &soc_before) {
  soc_before = rtc.soc;
  if (!r.ok) return SocEvent::NONE;

  if (!rtc.soc_valid) {
    // Aucune référence : première estimation par la tension, grossière sous charge.
    rtc.soc = socFromOcv(r.v);
    rtc.soc_valid = true;
    rtc.soc_est = true;  // jusqu'au premier recalage (pleine charge, repos, ou manuel)
    return SocEvent::INIT;
  }

  // Intégration trapézoïdale entre deux mesures. Au-delà de 6 h d'écart, la
  // moyenne de deux points ne vaut plus rien : on ne garde que la tension.
  if (dt_s > 0 && dt_s < 6 * 3600) {
    float i_mean = (rtc.sensor_ok ? (rtc.i + r.i) / 2 : r.i);
    double dq = (double)i_mean * dt_s;  // coulombs
    rtc.q_C += dq;
    double dah = dq / 3600.0;
    double eff;
    if (dah >= 0) {
      rtc.ah_in += dah;
      rtc.day.mah_in += dah * 1000;
      eff = dah * cfg.eta_chg;
    } else {
      rtc.ah_out += -dah;
      rtc.day.mah_out += -dah * 1000;
      float i20 = cfg.cap_ah / 20;
      eff = (-i_mean > i20) ? dah * pow(-i_mean / i20, cfg.peukert - 1) : dah;
    }
    rtc.soc = fmin(100.0, fmax(0.0, rtc.soc + eff / cfg.cap_ah * 100.0));
  }

  if (r.i > cfg.i_tail) rtc.last_charge = now;

  // Resynchronisation à 100 % (FR-05) : tension haute et courant de fin de charge.
  bool full_cond = r.v >= cfg.v_full && r.i >= -0.05f && r.i < cfg.i_tail;
  if (full_cond) {
    if (!rtc.full_since) rtc.full_since = now;
    else if (now - rtc.full_since >= cfg.full_hold_s && rtc.soc < 99.99) {
      rtc.soc = 100;
      rtc.soc_est = false;
      return SocEvent::FULL;
    }
  } else {
    rtc.full_since = 0;
  }

  // Recalage sur la tension à vide après un long repos.
  float i_rest = fmaxf(0.15f, cfg.cap_ah * 0.002f);
  if (fabsf(r.i) < i_rest) {
    if (!rtc.rest_since) {
      rtc.rest_since = now;
      rtc.rest_done = false;
    } else if (cfg.ocv_rest_s && !rtc.rest_done && now - rtc.rest_since >= cfg.ocv_rest_s) {
      rtc.rest_done = true;
      float target = socFromOcv(r.v - r.i * 0.018f);
      // Pondération : la table OCV est approximative, le comptage aussi. Sans
      // référence antérieure, la tension au repos fait foi.
      rtc.soc = rtc.soc_est ? target : rtc.soc + 0.5 * (target - rtc.soc);
      rtc.soc_est = false;
      return SocEvent::OCV;
    }
  } else {
    rtc.rest_since = 0;
  }
  return SocEvent::NONE;
}
