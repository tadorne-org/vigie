#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = ["pyserial>=3.5"]
# ///
"""Décode le journal d'un nœud Vigie (enregistrements de 32 octets, src/common/record.h).

    uv run tools/vigie_log.py vigie-log.bin                  # fichier téléchargé depuis /log.bin
    uv run tools/vigie_log.py --serial /dev/cu.wchusbserial110 # directement par l'USB (console « dump »)
    uv run tools/vigie_log.py --serial PORT --days            # résumés journaliers
    ... -o sortie.csv                                         # CSV au lieu de l'affichage

Horodatage : avant le premier recalage de l'horloge, `ts` compte les secondes
depuis la mise sous tension (drapeau time_valid = 0). L'enregistrement
`time_sync` qui suit porte le décalage appliqué ; l'outil l'ajoute à tous les
enregistrements non recalés du même démarrage (entre le `boot` précédent et ce
`time_sync`), qui deviennent ainsi datés. Ceux d'un démarrage jamais recalé
restent relatifs et sont signalés.
"""
import argparse
import csv
import struct
import sys
import time
from datetime import datetime, timezone

REC = struct.Struct("<IIiHHHhhHhHBBH")   # Record
DAY = struct.Struct("<IIIIHHHHhhBBH")    # DayRecord
TYPES = {1: "sample", 2: "boot", 3: "time_sync", 4: "soc_set", 5: "alert", 6: "config", 7: "cutoff"}
FLAGS = ["sim", "time_valid", "charging", "undervolt", "soc_sync", "sensor_err", "survival", "cutoff"]
ALERTS = {1: "soc_warn", 2: "soc_alarm", 3: "volt_low", 4: "volt_critical", 5: "temp_high", 6: "overcurrent",
          7: "no_charge_30d", 8: "boot_unexpected", 9: "sensor_lost", 10: "volt_cutoff"}
SOC_CAUSE = {1: "pleine charge", 2: "tension à vide", 3: "manuel", 4: "restauré"}
MODES = ["nav", "port", "mouillage", "hivernage"]
NO_TEMP = -32768


def crc16(b: bytes) -> int:
    crc = 0xFFFF
    for x in b:
        crc ^= x << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def valid(raw: bytes) -> bool:
    return len(raw) == 32 and raw[:4] != b"\xff\xff\xff\xff" and crc16(raw[:30]) == struct.unpack_from("<H", raw, 30)[0]


def decode(raw: bytes) -> dict:
    (seq, ts, i_avg, v_avg, v_min, v_max, i_min, i_max, soc, t, dt, type_mode, flags, _crc) = REC.unpack(raw)
    typ = type_mode & 0x0F
    r = {
        "seq": seq, "type": TYPES.get(typ, f"?{typ}"), "ts": ts, "time_valid": bool(flags & 2),
        "mode": MODES[(type_mode >> 4) & 3], "flags": "|".join(f for k, f in enumerate(FLAGS) if flags & (1 << k)),
        "v_avg": v_avg / 1000, "v_min": v_min / 1000, "v_max": v_max / 1000,
        "i_avg": i_avg / 1000, "i_min": i_min / 100, "i_max": i_max / 100,
        "soc": soc / 100, "t": None if t == NO_TEMP else t / 100, "dt_s": dt, "note": "",
    }
    if typ == 2:
        r["note"] = f"démarrage n°{i_avg}, cause de reset {dt}"
    elif typ == 3:
        r["note"] = f"horloge décalée de {i_avg:+d} s"
        r["offset"] = i_avg
    elif typ == 4:
        r["note"] = f"SOC recalé ({SOC_CAUSE.get(dt, dt)})"
    elif typ == 5:
        r["note"] = ALERTS.get(dt, f"alerte {dt}")
    elif typ == 7:
        r["note"] = "entrée en coupure" if dt else "sortie de coupure"
    return r


def decode_day(raw: bytes) -> dict:
    (seq, ts, mah_in, mah_out, v_min, v_max, soc_min, soc_max, t_min, t_max, flags, hours, _crc) = DAY.unpack(raw)
    return {
        "seq": seq, "ts": ts, "time_valid": bool(flags & 2), "hours": hours, "sim": bool(flags & 1),
        "ah_in": mah_in / 1000, "ah_out": mah_out / 1000, "ah_net": (mah_in - mah_out) / 1000,
        "v_min": v_min / 1000, "v_max": v_max / 1000, "soc_min": soc_min / 100, "soc_max": soc_max / 100,
        "t_min": None if t_min == NO_TEMP else t_min / 100, "t_max": None if t_max == NO_TEMP else t_max / 100,
    }


def fix_time(recs: list) -> None:
    """Recale les enregistrements relatifs d'après le time_sync de leur démarrage."""
    pending = []
    for r in recs:
        if r["type"] == "boot":
            pending = [r]
        elif r["type"] == "time_sync":
            for p in pending:
                p["ts"] += r["offset"]
                p["time_valid"] = True
                p["recalé"] = True
            pending = []
        elif not r["time_valid"]:
            pending.append(r)
    for r in recs:
        r["utc"] = (datetime.fromtimestamp(r["ts"], timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
                    if r["time_valid"] else f"+{r['ts']}s")


def read_serial(port: str, days: bool) -> list:
    import serial

    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, 115200, 0.5
    s.dtr = s.rts = False  # ne pas redémarrer la carte à l'ouverture
    s.open()
    s.reset_input_buffer()
    s.write(b"dump days\n" if days else b"dump\n")
    out, t0 = [], time.time()
    while time.time() - t0 < 600:
        line = s.readline().decode("ascii", "replace").strip()
        if line == "END":
            break
        if line[:2] in ("R ", "D "):
            out.append(bytes.fromhex(line[2:]))
            t0 = time.time()
        elif not line and not out and time.time() - t0 > 5:
            sys.exit("Pas de réponse : le nœud dort-il ? (appui sur BOOT pour ouvrir l'administration)")
    s.close()
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", nargs="?", help="fichier .bin (téléchargé depuis /log.bin)")
    ap.add_argument("--serial", help="port série du nœud")
    ap.add_argument("--days", action="store_true", help="résumés journaliers (avec --serial)")
    ap.add_argument("-o", "--output", help="écrit un CSV")
    a = ap.parse_args()

    if a.serial:
        raws = read_serial(a.serial, a.days)
    elif a.file:
        data = open(a.file, "rb").read()
        raws = [data[k:k + 32] for k in range(0, len(data) - 31, 32)]
    else:
        ap.error("un fichier ou --serial")
    good = [r for r in raws if valid(r)]
    if len(good) < len(raws):
        print(f"{len(raws) - len(good)} enregistrement(s) au CRC invalide ignoré(s)", file=sys.stderr)

    if a.days:
        recs = [decode_day(r) for r in good]
        for r in recs:
            r["utc"] = (datetime.fromtimestamp(r["ts"], timezone.utc).strftime("%Y-%m-%d")
                        if r["time_valid"] else f"+{r['ts']}s")
    else:
        recs = [decode(r) for r in good]
        fix_time(recs)

    if a.output:
        keys = list(dict.fromkeys(k for r in recs for k in r))
        with open(a.output, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=keys)
            w.writeheader()
            w.writerows(recs)
        print(f"{len(recs)} enregistrements → {a.output}")
        return

    for r in recs:
        if a.days:
            print(f"#{r['seq']:<5} {r['utc']:<12} {r['hours']:>2} h  +{r['ah_in']:.2f} / -{r['ah_out']:.2f} Ah  "
                  f"V {r['v_min']:.2f}–{r['v_max']:.2f}  SOC {r['soc_min']:.0f}–{r['soc_max']:.0f} %"
                  f"{'  SIM' if r['sim'] else ''}")
        elif r["type"] == "sample":
            t = "" if r["t"] is None else f" {r['t']:5.1f}°C"
            print(f"#{r['seq']:<6} {r['utc']:<19} {r['v_avg']:6.3f} V [{r['v_min']:.2f}–{r['v_max']:.2f}] "
                  f"{r['i_avg']:+7.3f} A  SOC {r['soc']:5.1f} %{t}  {r['dt_s']:>4} s  {r['flags']}")
        else:
            print(f"#{r['seq']:<6} {r['utc']:<19} {r['type'].upper():<10} {r['note']}")
    if not a.days and recs:
        rel = sum(1 for r in recs if not r["time_valid"])
        fixed = sum(1 for r in recs if r.get("recalé"))
        print(f"\n{len(recs)} enregistrements, {fixed} recalés a posteriori, {rel} sans date absolue.", file=sys.stderr)


if __name__ == "__main__":
    main()
