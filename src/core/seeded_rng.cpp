#include "nullspace/core/seeded_rng.hpp"

#include <cmath>
#include <numbers>

namespace nullspace::core {

// SplitMix64 finalizer: a bijection with strong avalanche properties.
// Flipping any single input bit changes each output bit with ~50% probability.
// Used to derive uncorrelated per-module seeds from a master seed.
//
// Reference: Sebastiano Vigna, "An experimental exploration of Marsaglia's
// xorshift generators, scrambled" (2017), adapted from the finalizer of
// Java's SplittableRandom.
uint64_t splitmix64(uint64_t x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x = x ^ (x >> 31);
    return x;
}

seeded_rng::seeded_rng(uint64_t seed)
    : seed_(seed), engine_(seed) {}

// Mix master_seed with module_id through splitmix64 to produce a seed
// that is uncorrelated with other modules' seeds. Adding or removing
// modules does not change any other module's seed.
seeded_rng seeded_rng::derive(uint64_t master_seed, uint64_t module_id) {
    uint64_t mixed = master_seed ^ (module_id * 0x9e3779b97f4a7c15ULL);
    return seeded_rng(splitmix64(mixed));
}

// Convert 64-bit engine output to a double in [0, 1).
//
// IEEE 754 double has a 53-bit significand. We take the top 53 bits
// of the engine output and scale by 2^(-53). This maps every possible
// 53-bit pattern to a unique, evenly-spaced double in [0, 1).
//
// Using our own conversion instead of std::uniform_real_distribution
// because the standard does not specify the conversion algorithm.
double seeded_rng::uniform() {
    uint64_t x = engine_();
    return static_cast<double>(x >> 11) * (1.0 / 9007199254740992.0);
}

double seeded_rng::uniform(double a, double b) {
    return a + (b - a) * uniform();
}

// Box-Muller transform: convert two uniform samples to two independent
// standard normal samples.
//
// Given u1, u2 ~ Uniform(0, 1):
//   z0 = sqrt(-2 ln(u1)) * cos(2π u2)
//   z1 = sqrt(-2 ln(u1)) * sin(2π u2)
//
// Then z0, z1 ~ N(0, 1) independently.
//
// Derivation: the joint density of two independent standard normals is
//   f(z0, z1) = (1/2π) exp(-(z0^2 + z1^2)/2)
// which is radially symmetric. In polar coordinates (r, θ):
//   r^2 = -2 ln(u1)   (inverse CDF of the Rayleigh distribution)
//   θ = 2π u2          (uniform angle)
//
// We generate both z0 and z1 but return z0 immediately and cache z1
// as the spare for the next call. This halves the number of uniform
// samples consumed.
double seeded_rng::gaussian(double mean, double stddev) {
    if (has_spare_) {
        has_spare_ = false;
        return mean + stddev * spare_;
    }

    double u1, u2;
    do { u1 = uniform(); } while (u1 == 0.0);
    u2 = uniform();

    double r = std::sqrt(-2.0 * std::log(u1));
    double theta = 2.0 * std::numbers::pi * u2;

    spare_ = r * std::sin(theta);
    has_spare_ = true;

    return mean + stddev * r * std::cos(theta);
}

std::mt19937_64& seeded_rng::engine() {
    return engine_;
}

uint64_t seeded_rng::seed() const {
    return seed_;
}

} // namespace nullspace::core
