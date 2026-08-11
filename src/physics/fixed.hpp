#pragma once

#include <cstdint>
#include <compare>

// Deterministic fixed-point arithmetic. No float appears in any simulation path:
// float results vary with compiler, optimization level and FMA contraction, which
// would desync two machines running identical inputs.
//
// int64 raw with 20 fractional bits:
//   range     +/- 2^43  = +/- 8,796,093,022,208 world units
//   precision   1/2^20  = 0.00000095 world units
//
// Quantization error is identical on every machine, so it costs fidelity but never
// determinism.

namespace phys {

class fixed {
public:
    static constexpr int     k_frac_bits = 20;
    static constexpr int64_t k_one_raw   = int64_t{1} << k_frac_bits;

    constexpr fixed() = default;

    static constexpr fixed from_int(int64_t v) { return fixed{v << k_frac_bits}; }
    static constexpr fixed from_raw(int64_t v) { return fixed{v}; }

    // Compile-time constants and external input only - never inside the simulation.
    static constexpr fixed from_float(float v) {
        return fixed{static_cast<int64_t>(v * static_cast<float>(k_one_raw))};
    }

    constexpr int64_t raw() const    { return _raw; }
    constexpr int64_t to_int() const { return _raw >> k_frac_bits; }
    constexpr float to_float() const {
        return static_cast<float>(_raw) / static_cast<float>(k_one_raw);
    }

    constexpr fixed operator-() const { return fixed{-_raw}; }
    constexpr fixed operator+() const { return *this; }

    constexpr fixed& operator+=(fixed o) { _raw += o._raw; return *this; }
    constexpr fixed& operator-=(fixed o) { _raw -= o._raw; return *this; }
    constexpr fixed& operator*=(fixed o) { *this = *this * o; return *this; }
    constexpr fixed& operator/=(fixed o) { *this = *this / o; return *this; }

    constexpr fixed& operator*=(int64_t s) { *this = *this * s; return *this; }
    constexpr fixed& operator/=(int64_t s) { *this = *this / s; return *this; }

    friend constexpr fixed operator+(fixed a, fixed b) { return fixed{a._raw + b._raw}; }
    friend constexpr fixed operator-(fixed a, fixed b) { return fixed{a._raw - b._raw}; }

    // 128-bit intermediate: two 43.20 values produce 86.40 before the shift back down.
    friend constexpr fixed operator*(fixed a, fixed b) {
        return fixed{static_cast<int64_t>(
            (static_cast<__int128>(a._raw) * static_cast<__int128>(b._raw)) >> k_frac_bits)};
    }

    friend constexpr fixed operator/(fixed a, fixed b) {
        return fixed{static_cast<int64_t>(
            (static_cast<__int128>(a._raw) << k_frac_bits) / static_cast<__int128>(b._raw))};
    }

    // Scaling by a plain count - no shift, unlike fixed * fixed.
    friend constexpr fixed operator*(fixed a, int64_t s) {
        return fixed{static_cast<int64_t>(static_cast<__int128>(a._raw) * s)};
    }
    friend constexpr fixed operator*(int64_t s, fixed a) { return a * s; }
    friend constexpr fixed operator/(fixed a, int64_t s) { return fixed{a._raw / s}; }

    // Raw ordering is correct for fixed-point.
    friend constexpr auto operator<=>(fixed, fixed) = default;
    friend constexpr bool operator==(fixed, fixed) = default;

private:
    explicit constexpr fixed(int64_t raw) : _raw(raw) {}

    int64_t _raw{0};
};

// Integer Newton's method - deterministic, unlike std::sqrt.
constexpr uint64_t isqrt(unsigned __int128 n) {
    if (n == 0) return 0;

    unsigned __int128 x = n;
    unsigned __int128 y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }

    return static_cast<uint64_t>(x);
}

} // namespace phys
