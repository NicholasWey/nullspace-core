#pragma once

#include <cstdint>
#include <random>

namespace nullspace::core {

uint64_t splitmix64(uint64_t x);

// Deterministic PRNG with cross-platform identical output.
//
// mt19937_64 produces the same sequence on every conforming compiler.
// std::uniform_real_distribution and std::normal_distribution do NOT.
// This class implements its own uniform and Box-Muller transforms so
// that the same seed produces bit-identical output on MSVC and GCC.
class seeded_rng {
public:
    explicit seeded_rng(uint64_t seed);

    static seeded_rng derive(uint64_t master_seed, uint64_t module_id);

    double uniform();
    double uniform(double a, double b);
    double gaussian(double mean, double stddev);

    std::mt19937_64& engine();
    uint64_t seed() const;

private:
    uint64_t seed_;
    std::mt19937_64 engine_;
    bool has_spare_ = false;
    double spare_ = 0.0;
};

} // namespace nullspace::core
