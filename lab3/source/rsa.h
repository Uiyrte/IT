#pragma once
#include <cstdint>
#include <stdexcept>

namespace rsa {

inline int64_t gcd(int64_t a, int64_t b) {
    while (b) { a %= b; std::swap(a, b); }
    return a;
}

inline int64_t extended_gcd(int64_t a, int64_t b, int64_t &x, int64_t &y) {
    int64_t old_r = a, r = b;
    int64_t old_s = 1, s = 0;
    int64_t old_t = 0, t = 1;
    while (r != 0) {
        int64_t q     = old_r / r;
        int64_t new_r = old_r - q * r;
        int64_t new_s = old_s - q * s;
        int64_t new_t = old_t - q * t;
        old_r = r; r = new_r;
        old_s = s; s = new_s;
        old_t = t; t = new_t;
    }
    x = old_s;
    y = old_t;
    return old_r;
}

inline int64_t mod_inverse(int64_t a, int64_t m) {
    int64_t x, y;
    int64_t g = extended_gcd(a, m, x, y);
    if (g != 1) throw std::runtime_error("Обратный элемент не существует");
    return (x % m + m) % m;
}

inline int64_t power_mod(int64_t base, int64_t exp, int64_t mod) {
    int64_t result = 1;
    int64_t b = base % mod;
    while (exp > 0) {
        if (exp % 2 != 0) {
            result = result * b % mod;
            exp -= 1;
        } else {
            b = b * b % mod;
            exp /= 2;
        }
    }
    return result;
}

inline bool is_prime(int64_t n) {
    if (n < 2)       return false;
    if (n == 2)      return true;
    if (n % 2 == 0)  return false;
    for (int64_t i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

}
