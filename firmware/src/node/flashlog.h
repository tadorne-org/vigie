// Journal circulaire sur la partition brute « vlog » (partitions_vigie.csv).
//
// Pas de système de fichiers : des secteurs de 4 Ko remplis d'enregistrements
// de 32 octets (128 par secteur), numérotés de façon monotone. Quand l'anneau
// est plein, le secteur le plus ancien est effacé en entier. Au démarrage,
// l'en-tête de chaque secteur suffit à retrouver la tête d'écriture ; un
// enregistrement coupé par une perte d'alimentation échoue au CRC et est
// simplement ignoré à la lecture.
//
// Usure : à un enregistrement toutes les 5 min, chaque secteur est effacé une
// fois tous les ~3 mois — 100 000 cycles garantis, soit bien plus que la vie
// de la carte.
#pragma once
#include <esp_partition.h>
#include <stdint.h>

#include "record.h"

class FlashRing {
 public:
  // `key` : clé NVS où survit la numérotation quand l'anneau est effacé.
  bool begin(const esp_partition_t *part, uint32_t first_sector, uint32_t sectors, const char *key);
  bool append(void *rec);  // affecte seq et crc
  bool read(uint32_t seq, void *rec) const;
  uint32_t firstSeq() const { return first_seq_; }
  uint32_t nextSeq() const { return next_seq_; }
  uint32_t count() const { return next_seq_ - first_seq_; }
  uint32_t capacity() const { return (sectors_ - 1) * kPerSector; }  // un secteur sacrifié à l'effacement
  bool erase();

  static constexpr uint32_t kSector = 4096;
  static constexpr uint32_t kPerSector = kSector / vlog::kRecordSize;

 private:
  uint32_t addr(uint32_t sector, uint32_t slot) const { return (base_ + sector) * kSector + slot * vlog::kRecordSize; }
  const esp_partition_t *part_ = nullptr;
  const char *key_ = nullptr;
  uint32_t base_ = 0, sectors_ = 0;
  uint32_t head_sector_ = 0, head_slot_ = 0;  // prochain emplacement libre
  uint32_t first_seq_ = 1, next_seq_ = 1;
  // Pour retrouver un seq : secteur qui contient first_seq_ (aligné en tête de secteur)
  uint32_t first_sector_ = 0;
  bool head_needs_erase_ = true;
};

namespace flashlog {
bool begin();
extern FlashRing samples;
extern FlashRing days;
}  // namespace flashlog
