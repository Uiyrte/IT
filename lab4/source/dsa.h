#pragma once
#include <cstdint>
#include <vector>

namespace dsa {

// Быстрое возведение в степень по модулю: base^exp mod m
inline int64_t power_mod(int64_t base, int64_t exp, int64_t mod) {
    if (mod == 1) return 0;
    int64_t result = 1;
    base = ((base % mod) + mod) % mod;
    while (exp > 0) {
        if (exp & 1)
            result = static_cast<int64_t>((__int128)result * base % mod);
        base = static_cast<int64_t>((__int128)base * base % mod);
        exp >>= 1;
    }
    return result;
}

// Обратный элемент по малой теореме Ферма: a^(q-2) mod q  (q — простое)
inline int64_t mod_inverse(int64_t a, int64_t q) {
    return power_mod(a, q - 2, q);
}

// Проверка простоты (пробные делители)
inline bool is_prime(int64_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int64_t i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

struct HashStep {
    int     i;
    int64_t h_prev;
    int64_t mi;
    int64_t sum;   // (h_prev + mi) % q
    int64_t h_next;
};

// Хеш-функция 3.2:  Hᵢ = (Hᵢ₋₁ + Mᵢ)² mod q,  H₀ задаётся пользователем
// Если steps != nullptr — заполняет вектор шагов
inline int64_t hash_message(const std::vector<uint8_t>& data, int64_t q, int64_t h0 = 100,
                             std::vector<HashStep>* steps = nullptr) {
    int64_t h = h0 % q;
    for (int i = 0; i < (int)data.size(); i++) {
        int64_t mi = static_cast<int64_t>(data[i]);
        int64_t s  = (h + mi) % q;
        int64_t hn = static_cast<int64_t>((__int128)s * s % q);
        if (steps) steps->push_back({i + 1, h, mi, s, hn});
        h = hn;
    }
    return h;
}

} // namespace dsa
