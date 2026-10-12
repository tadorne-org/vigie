#include "flashlog.h"

#include <Preferences.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "vlog";

namespace flashlog {
FlashRing samples;
FlashRing days;

constexpr uint32_t kDaySectors = 8;  // 7 × 128 = 896 jours garantis

bool begin() {
  const esp_partition_t *p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x40, "vlog");
  if (!p) {
    ESP_LOGE(TAG, "partition vlog absente (table de partitions ?)");
    return false;
  }
  uint32_t total = p->size / FlashRing::kSector;
  return days.begin(p, 0, kDaySectors, "seq_days") && samples.begin(p, kDaySectors, total - kDaySectors, "seq_samples");
}
}  // namespace flashlog

static uint32_t loadSeqBase(const char *key) {
  Preferences p;
  if (!p.begin("vlog", false)) return 1;
  uint32_t v = p.getUInt(key, 1);
  p.end();
  return v ? v : 1;
}

static void saveSeqBase(const char *key, uint32_t v) {
  Preferences p;
  if (!p.begin("vlog", false)) return;
  p.putUInt(key, v);
  p.end();
}

bool FlashRing::begin(const esp_partition_t *part, uint32_t first_sector, uint32_t sectors, const char *key) {
  part_ = part;
  key_ = key;
  base_ = first_sector;
  sectors_ = sectors;

  // Premier enregistrement valide de chaque secteur : un secteur est soit
  // effacé, soit commence par un seq aligné.
  uint32_t best_seq = 0, best_sector = 0, low_seq = UINT32_MAX, low_sector = 0;
  bool any = false;
  vlog::Record r;
  for (uint32_t s = 0; s < sectors_; s++) {
    if (esp_partition_read(part_, addr(s, 0), &r, sizeof r) != ESP_OK) return false;
    if (!vlog::valid(r)) continue;
    any = true;
    if (r.seq >= best_seq) { best_seq = r.seq; best_sector = s; }
    if (r.seq < low_seq) { low_seq = r.seq; low_sector = s; }
  }
  if (!any) {
    head_sector_ = 0;
    head_slot_ = 0;
    first_seq_ = next_seq_ = loadSeqBase(key_);
    first_sector_ = 0;
    head_needs_erase_ = true;
    return true;
  }
  // Tête : premier emplacement effacé du secteur le plus récent.
  uint32_t last = best_seq;
  uint32_t slot = 1;
  for (; slot < kPerSector; slot++) {
    if (esp_partition_read(part_, addr(best_sector, slot), &r, sizeof r) != ESP_OK) return false;
    if (r.seq == 0xFFFFFFFF) break;
    // Emplacement écrit, même corrompu : il occupe son numéro.
    last = best_seq + slot;
  }
  head_sector_ = best_sector;
  head_slot_ = slot;
  head_needs_erase_ = false;
  next_seq_ = last + 1;
  first_seq_ = low_seq;
  first_sector_ = low_sector;
  ESP_LOGI(TAG, "anneau @%u : %u enregistrements (%u..%u)", (unsigned)base_, (unsigned)count(), (unsigned)first_seq_,
           (unsigned)(next_seq_ - 1));
  return true;
}

bool FlashRing::append(void *rec) {
  if (!part_) return false;
  if (head_slot_ >= kPerSector) {
    head_sector_ = (head_sector_ + 1) % sectors_;
    head_slot_ = 0;
    head_needs_erase_ = true;
  }
  if (head_needs_erase_) {
    // Effacer ce secteur fait disparaître les plus anciens enregistrements.
    if (count() > 0 && head_sector_ == first_sector_) {
      first_seq_ += kPerSector;
      first_sector_ = (first_sector_ + 1) % sectors_;
    }
    if (esp_partition_erase_range(part_, addr(head_sector_, 0), kSector) != ESP_OK) return false;
    head_needs_erase_ = false;
    if (count() == 0) {
      first_seq_ = next_seq_;
      first_sector_ = head_sector_;
    }
  }
  vlog::Record *r = (vlog::Record *)rec;
  r->seq = next_seq_;
  vlog::seal(*r);
  if (esp_partition_write(part_, addr(head_sector_, head_slot_), r, vlog::kRecordSize) != ESP_OK) return false;
  head_slot_++;
  next_seq_++;
  return true;
}

bool FlashRing::read(uint32_t seq, void *rec) const {
  if (!part_ || seq < first_seq_ || seq >= next_seq_) return false;
  uint32_t off = seq - first_seq_;
  uint32_t sector = (first_sector_ + off / kPerSector) % sectors_;
  uint32_t slot = off % kPerSector;
  if (esp_partition_read(part_, addr(sector, slot), rec, vlog::kRecordSize) != ESP_OK) return false;
  const vlog::Record *r = (const vlog::Record *)rec;
  return vlog::valid(*r) && r->seq == seq;
}

bool FlashRing::erase() {
  if (!part_) return false;
  if (esp_partition_erase_range(part_, base_ * kSector, sectors_ * kSector) != ESP_OK) return false;
  // On garde la numérotation : un récepteur ne doit jamais revoir un seq déjà vu.
  saveSeqBase(key_, next_seq_);
  first_seq_ = next_seq_;
  head_sector_ = 0;
  head_slot_ = 0;
  first_sector_ = 0;
  head_needs_erase_ = false;
  return true;
}
