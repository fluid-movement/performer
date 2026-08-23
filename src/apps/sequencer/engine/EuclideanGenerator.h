#pragma once

#include <algorithm>

// Bresenham Euclidean rhythm generator, canonical form.
// Rotates so the first hit is at index 0, then applies user rotation r.
// out[i] = true if a pulse falls on step i (sequence length n, k pulses).
inline void computeEuclidean(bool *out, int n, int k, int r) {
    int bucket = 0;
    bool raw[16] = {};
    n = std::max(n, 1);
    k = std::min(k, n);
    for (int i = 0; i < n; i++) {
        bucket += k;
        if (bucket >= n) { bucket -= n; raw[i] = true; }
    }
    int firstHit = 0;
    if (k > 0) {
        for (int i = 0; i < n; i++) { if (raw[i]) { firstHit = i; break; } }
    }
    for (int i = 0; i < n; i++)
        out[i] = raw[((i + firstHit - r) % n + n) % n];
}
