// Console série (115200 bauds), active en régime éveillé.
#include <Arduino.h>

#include "app.h"
#include "config.h"
#include "flashlog.h"
#include "record.h"
#include "state.h"

namespace cli {

static char line[160];
static size_t len = 0;

static void help() {
  Serial.println(
      "Commandes :\n"
      "  status                 état courant (JSON)\n"
      "  config                 paramètres\n"
      "  schema                 schéma et valeurs des paramètres (JSON de la page web)\n"
      "  set <clé> <valeur>     modifie et enregistre un paramètre\n"
      "  log [n]                n derniers enregistrements (défaut 10)\n"
      "  days [n]               n derniers résumés journaliers\n"
      "  dump [days] [depuis]   journal brut en hex (tools/vigie_log.py --serial)\n"
      "  sample | push          mesure / push immédiat\n"
      "  time <epoch>           règle l'horloge (UTC)\n"
      "  soc <0-100>            recale le SOC\n"
      "  sim <scénario> [soc]   0 port, 1 nav, 2 hivernage, 3 décharge, 4 quai\n"
      "  stay <s>               prolonge l'administration\n"
      "  sleep                  ferme l'administration et dort\n"
      "  erase EFFACER          efface le journal\n"
      "  reboot");
}

static void printRecord(const vlog::Record &r) {
  static const char *types[] = {"?", "sample", "boot", "time_sync", "soc_set", "alert", "config", "cutoff"};
  uint8_t t = r.type_mode & 0x0F;
  Serial.printf("#%-6lu %-9s ts=%-10lu%s v=%.3f [%.3f..%.3f] i=%+.3f [%+.2f..%+.2f] soc=%.2f", (unsigned long)r.seq,
                t < 8 ? types[t] : "?", (unsigned long)r.ts, (r.flags & vlog::RF_TIME_VALID) ? "" : "*",
                r.v_avg_mV / 1000.0, r.v_min_mV / 1000.0, r.v_max_mV / 1000.0, r.i_avg_mA / 1000.0,
                r.i_min_cA / 100.0, r.i_max_cA / 100.0, r.soc_cp / 100.0);
  if (r.t_cC != vlog::kNoTemp) Serial.printf(" t=%.2f", r.t_cC / 100.0);
  Serial.printf(" dt=%u flags=0x%02X%s\n", r.dt_s, r.flags, (r.flags & vlog::RF_SIM) ? " SIM" : "");
  if (t == vlog::REC_ALERT) Serial.printf("        alerte %s\n", vlog::alertName(r.dt_s));
}

static void exec(char *cmd) {
  char *arg = strchr(cmd, ' ');
  if (arg) {
    *arg++ = 0;
    while (*arg == ' ') arg++;
  }
  admin::extend(120);
  if (!strcmp(cmd, "help") || !strcmp(cmd, "?")) help();
  else if (!strcmp(cmd, "status")) Serial.println(app::statusJson());
  else if (!strcmp(cmd, "schema")) Serial.println(configJson());
  else if (!strcmp(cmd, "config")) {
    for (size_t k = 0; k < kFieldCount; k++)
      Serial.printf("  %-14s = %s\n", kFields[k].key, kFields[k].secret ? "***" : configGet(kFields[k]).c_str());
  } else if (!strcmp(cmd, "set") && arg) {
    char *val = strchr(arg, ' ');
    const char *v = "";
    if (val) {
      *val++ = 0;
      v = val;
    }
    if (configSet(arg, v) && configSave()) {
      app::configChanged();
      Serial.println("ok");
    } else {
      Serial.println("refusé (clé inconnue ou valeur hors bornes)");
    }
  } else if (!strcmp(cmd, "log") || !strcmp(cmd, "days")) {
    bool days = cmd[0] == 'd';
    FlashRing &ring = days ? flashlog::days : flashlog::samples;
    uint32_t n = arg ? atoi(arg) : 10;
    uint32_t s = ring.nextSeq() > n ? max(ring.nextSeq() - n, ring.firstSeq()) : ring.firstSeq();
    Serial.printf("%lu enregistrements (#%lu..#%lu)\n", (unsigned long)ring.count(), (unsigned long)ring.firstSeq(),
                  (unsigned long)ring.nextSeq() - 1);
    for (; s < ring.nextSeq(); s++) {
      if (days) {
        vlog::DayRecord d;
        if (!ring.read(s, &d)) continue;
        Serial.printf("#%-4lu ts=%lu h=%u in=%.3f Ah out=%.3f Ah v=[%.3f..%.3f] soc=[%.1f..%.1f]\n",
                      (unsigned long)d.seq, (unsigned long)d.ts, d.hours, d.mah_in / 1000.0, d.mah_out / 1000.0,
                      d.v_min_mV / 1000.0, d.v_max_mV / 1000.0, d.soc_min_cp / 100.0, d.soc_max_cp / 100.0);
      } else {
        vlog::Record r;
        if (ring.read(s, &r)) printRecord(r);
        else Serial.printf("#%lu illisible\n", (unsigned long)s);
      }
    }
  } else if (!strcmp(cmd, "dump")) {
    bool days = arg && !strncmp(arg, "days", 4);
    if (days) arg = strchr(arg, ' ');
    FlashRing &ring = days ? flashlog::days : flashlog::samples;
    uint32_t from = arg ? max<uint32_t>(atoi(arg), ring.firstSeq()) : ring.firstSeq();
    uint8_t rec[vlog::kRecordSize];
    char hex[2 * vlog::kRecordSize + 1];
    for (uint32_t s = from; s < ring.nextSeq(); s++) {
      if (!ring.read(s, rec)) continue;
      for (int k = 0; k < vlog::kRecordSize; k++) snprintf(hex + 2 * k, 3, "%02x", rec[k]);
      Serial.printf("%c %s\n", days ? 'D' : 'R', hex);
      if ((s & 63) == 0) admin::extend(60);
    }
    Serial.println("END");
  } else if (!strcmp(cmd, "sample")) app::sampleNow();
  else if (!strcmp(cmd, "push")) app::pushNow();
  else if (!strcmp(cmd, "time") && arg) app::setTime(atoll(arg), "console");
  else if (!strcmp(cmd, "soc") && arg) app::setSoc(atof(arg));
  else if (!strcmp(cmd, "sim") && arg) {
    char *soc = strchr(arg, ' ');
    if (soc) *soc++ = 0;
    if (configSet("sim_scn", arg)) configSave();
    if (soc) app::simReset(atof(soc));
    app::sampleNow();
    Serial.println("ok");
  } else if (!strcmp(cmd, "stay")) admin::extend(arg ? atoi(arg) : 900);
  else if (!strcmp(cmd, "sleep")) app::sleepNow();
  else if (!strcmp(cmd, "erase") && arg && !strcmp(arg, "EFFACER")) Serial.println(app::eraseLogs() ? "effacé" : "échec");
  else if (!strcmp(cmd, "reboot")) ESP.restart();
  else if (*cmd) Serial.println("? (help)");
}

void poll() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (len) {
        line[len] = 0;
        exec(line);
        len = 0;
      }
    } else if (len < sizeof line - 1) {
      line[len++] = c;
    }
  }
}

}  // namespace cli
