// Encodeur / lecteur CBOR minimal (RFC 8949), sans allocation.
//
// Suffisant pour le bus TsubameBus : cartes à clés texte, entiers, flottants,
// chaînes d'octets. L'encodeur écrit des cartes de longueur indéfinie
// (0xBF … 0xFF) pour ne pas avoir à compter les champs à l'avance.
#pragma once
#include <stdint.h>
#include <string.h>

class CborWriter {
 public:
  CborWriter(uint8_t *buf, size_t cap) : buf_(buf), cap_(cap) {}

  size_t size() const { return len_; }
  bool ok() const { return !overflow_; }

  void mapBegin() { put(0xBF); }
  void mapEnd() { put(0xFF); }
  void arrayBegin() { put(0x9F); }
  void arrayEnd() { put(0xFF); }

  void uint(uint64_t v) { head(0, v); }
  void sint(int64_t v) {
    if (v >= 0) head(0, (uint64_t)v);
    else head(1, (uint64_t)(-1 - v));
  }
  void text(const char *s) {
    size_t n = strlen(s);
    head(3, n);
    raw(s, n);
  }
  void bytes(const void *p, size_t n) {
    head(2, n);
    raw(p, n);
  }
  void f32(float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    put(0xFA);
    for (int s = 24; s >= 0; s -= 8) put(u >> s);
  }
  void f64(double d) {
    uint64_t u;
    memcpy(&u, &d, 8);
    put(0xFB);
    for (int s = 56; s >= 0; s -= 8) put(u >> s);
  }
  void boolean(bool b) { put(b ? 0xF5 : 0xF4); }

  // Raccourcis clé + valeur
  void kv(const char *k, uint64_t v) { text(k); uint(v); }
  void kvi(const char *k, int64_t v) { text(k); sint(v); }
  void kvf(const char *k, float v) { text(k); f32(v); }
  void kvd(const char *k, double v) { text(k); f64(v); }
  void kvs(const char *k, const char *v) { text(k); text(v); }

 private:
  void put(uint8_t b) {
    if (len_ < cap_) buf_[len_++] = b;
    else overflow_ = true;
  }
  void raw(const void *p, size_t n) {
    if (len_ + n > cap_) { overflow_ = true; return; }
    memcpy(buf_ + len_, p, n);
    len_ += n;
  }
  void head(uint8_t major, uint64_t v) {
    uint8_t m = major << 5;
    if (v < 24) put(m | v);
    else if (v <= 0xFF) { put(m | 24); put(v); }
    else if (v <= 0xFFFF) { put(m | 25); put(v >> 8); put(v); }
    else if (v <= 0xFFFFFFFFull) { put(m | 26); for (int s = 24; s >= 0; s -= 8) put(v >> s); }
    else { put(m | 27); for (int s = 56; s >= 0; s -= 8) put(v >> s); }
  }

  uint8_t *buf_;
  size_t cap_;
  size_t len_ = 0;
  bool overflow_ = false;
};

// Lecteur : parcourt une carte au premier niveau et rend la valeur numérique
// d'une clé texte. Les valeurs imbriquées sont sautées.
class CborReader {
 public:
  CborReader(const uint8_t *p, size_t n) : p_(p), n_(n) {}

  // Cherche `key` dans la carte racine ; rend vrai si trouvée et numérique.
  bool findNumber(const char *key, double &out) {
    pos_ = 0;
    uint8_t ib;
    if (!byte(ib)) return false;
    uint8_t major = ib >> 5;
    if (major != 5) return false;
    bool indef = (ib & 0x1F) == 31;
    uint64_t count = 0;
    if (!indef && !arg(ib, count)) return false;
    size_t klen = strlen(key);
    for (uint64_t i = 0; indef || i < count; i++) {
      if (indef && pos_ < n_ && p_[pos_] == 0xFF) return false;
      // Clé
      if (!byte(ib)) return false;
      bool match = false;
      if ((ib >> 5) == 3) {
        uint64_t len;
        if (!arg(ib, len) || pos_ + len > n_) return false;
        match = (len == klen && memcmp(p_ + pos_, key, klen) == 0);
        pos_ += len;
      } else {
        pos_--;
        if (!skip()) return false;
      }
      // Valeur
      if (match) return number(out);
      if (!skip()) return false;
    }
    return false;
  }

 private:
  bool byte(uint8_t &b) {
    if (pos_ >= n_) return false;
    b = p_[pos_++];
    return true;
  }
  bool arg(uint8_t ib, uint64_t &v) {
    uint8_t ai = ib & 0x1F;
    if (ai < 24) { v = ai; return true; }
    int nb = ai == 24 ? 1 : ai == 25 ? 2 : ai == 26 ? 4 : ai == 27 ? 8 : 0;
    if (!nb || pos_ + nb > n_) return false;
    v = 0;
    for (int i = 0; i < nb; i++) v = (v << 8) | p_[pos_++];
    return true;
  }
  bool number(double &out) {
    uint8_t ib;
    if (!byte(ib)) return false;
    uint8_t major = ib >> 5;
    uint64_t v;
    if (major == 0) { if (!arg(ib, v)) return false; out = (double)v; return true; }
    if (major == 1) { if (!arg(ib, v)) return false; out = -1.0 - (double)v; return true; }
    if (ib == 0xFA && pos_ + 4 <= n_) {
      uint32_t u = 0;
      for (int i = 0; i < 4; i++) u = (u << 8) | p_[pos_++];
      float f;
      memcpy(&f, &u, 4);
      out = f;
      return true;
    }
    if (ib == 0xFB && pos_ + 8 <= n_) {
      uint64_t u = 0;
      for (int i = 0; i < 8; i++) u = (u << 8) | p_[pos_++];
      memcpy(&out, &u, 8);
      return true;
    }
    return false;
  }
  bool skip(int depth = 0) {
    if (depth > 8) return false;
    uint8_t ib;
    if (!byte(ib)) return false;
    uint8_t major = ib >> 5, ai = ib & 0x1F;
    if (major == 7) {
      int extra = ai == 25 ? 2 : ai == 26 ? 4 : ai == 27 ? 8 : ai == 24 ? 1 : 0;
      pos_ += extra;
      return pos_ <= n_;
    }
    if (ai == 31) {  // longueur indéfinie
      while (pos_ < n_ && p_[pos_] != 0xFF) {
        if (!skip(depth + 1)) return false;
        if (major == 5 && !skip(depth + 1)) return false;
      }
      pos_++;
      return pos_ <= n_;
    }
    uint64_t v;
    if (!arg(ib, v)) return false;
    switch (major) {
      case 0: case 1: return true;
      case 2: case 3: pos_ += v; return pos_ <= n_;
      case 4: for (uint64_t i = 0; i < v; i++) if (!skip(depth + 1)) return false; return true;
      case 5: for (uint64_t i = 0; i < 2 * v; i++) if (!skip(depth + 1)) return false; return true;
      case 6: return skip(depth + 1);
    }
    return false;
  }

  const uint8_t *p_;
  size_t n_;
  size_t pos_ = 0;
};
