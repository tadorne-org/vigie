#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = ["bleak>=0.22", "cryptography>=42"]
# ///
"""Écoute les annonces BTHome v2 d'un nœud Vigie et les décode.

    uv run tools/vigie_ble.py                 # toutes les annonces BTHome
    uv run tools/vigie_ble.py --key 0011…ff   # déchiffre (clé ble_key du nœud)
    uv run tools/vigie_ble.py --duration 120

Sous macOS, le terminal doit avoir l'autorisation Bluetooth (Réglages système
> Confidentialité et sécurité > Bluetooth). macOS masque les adresses MAC : le
déchiffrement a besoin de la MAC Bluetooth du nœud : Vigie la publie dans le
bloc constructeur (0xFFFF) de sa réponse au scan, --mac reste possible.
"""
import argparse
import asyncio
import struct
import time

from bleak import BleakScanner

UUID = "0000fcd2-0000-1000-8000-00805f9b34fb"

# id: (nom, format struct, facteur, unité)
OBJECTS = {
    0x00: ("packet", "<B", 1, ""),
    0x01: ("battery", "<B", 1, "%"),
    0x02: ("temperature", "<h", 0.01, "°C"),
    0x0C: ("voltage", "<H", 0.001, "V"),
    0x5C: ("power", "<i", 0.01, "W"),
    0x5D: ("current", "<h", 0.001, "A"),
}


def decode_objects(data: bytes) -> dict:
    out, k = {}, 0
    while k < len(data):
        oid = data[k]
        if oid not in OBJECTS:
            out["?"] = data[k:].hex()
            break
        name, fmt, factor, unit = OBJECTS[oid]
        n = struct.calcsize(fmt)
        (v,) = struct.unpack_from(fmt, data, k + 1)
        out[name] = round(v * factor, 3)
        k += 1 + n
    return out


def decrypt(sd: bytes, key: bytes, mac: bytes) -> bytes:
    from cryptography.hazmat.primitives.ciphers.aead import AESCCM

    devinfo, payload = sd[0], sd[1:]
    ciphertext, counter, mic = payload[:-8], payload[-8:-4], payload[-4:]
    nonce = mac + bytes([0xD2, 0xFC, devinfo]) + counter
    return AESCCM(key, tag_length=4).decrypt(nonce, ciphertext + mic, None)


async def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--key", help="clé BTHome, 32 caractères hex")
    ap.add_argument("--mac", help="MAC Bluetooth du nœud (aa:bb:cc:dd:ee:ff), requise pour déchiffrer sous macOS")
    ap.add_argument("--duration", type=float, default=60)
    ap.add_argument("--all", action="store_true", help="afficher aussi les appareils BTHome nommés autrement que Vigie")
    a = ap.parse_args()
    key = bytes.fromhex(a.key) if a.key else None
    seen = {}

    def cb(dev, adv):
        sd = adv.service_data.get(UUID)
        if not sd:
            return
        name = adv.local_name or dev.name or seen.get(dev.address, "")
        seen[dev.address] = name
        # Le nom n'arrive qu'avec la réponse au scan : une annonce anonyme d'un
        # appareil pas encore nommé est affichée sous son adresse.
        if not a.all and name and not name.startswith("Vigie"):
            return
        devinfo = sd[0]
        enc = devinfo & 1
        try:
            if enc:
                if not key:
                    obj = {"chiffré": sd[1:].hex()}
                else:
                    md = adv.manufacturer_data.get(0xFFFF, b"")
                    # Bloc constructeur Vigie : 'V', version, MAC, drapeaux, alertes
                    auto = md[2:8] if len(md) >= 12 and md[0] == ord("V") else None
                    mac = auto or bytes.fromhex((a.mac or dev.address).replace(":", ""))
                    obj = decode_objects(decrypt(sd, key, mac))
                    obj["counter"] = struct.unpack("<I", sd[-8:-4])[0]
            else:
                obj = decode_objects(sd[1:])
        except Exception as e:  # noqa: BLE001
            obj = {"erreur": str(e)}
        print(f"{time.strftime('%H:%M:%S')} {seen[dev.address] or dev.address:<16} rssi={adv.rssi:>4} "
              f"v{devinfo >> 5}{' chiffré' if enc else ''} {obj}", flush=True)

    async with BleakScanner(cb, scanning_mode="active"):
        await asyncio.sleep(a.duration)


if __name__ == "__main__":
    asyncio.run(main())
