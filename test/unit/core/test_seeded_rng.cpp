#include "nullspace/core/seeded_rng.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <numeric>
#include <vector>

using nullspace::core::seeded_rng;
using nullspace::core::splitmix64;

// Same seed must produce the same sequence every time.
TEST(SeededRng, Deterministic) {
    seeded_rng a(42);
    seeded_rng b(42);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(a.uniform(), b.uniform());
    }
}

TEST(SeededRng, DeterministicGaussian) {
    seeded_rng a(123);
    seeded_rng b(123);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(a.gaussian(0.0, 1.0), b.gaussian(0.0, 1.0));
    }
}

// Different seeds must produce different sequences.
TEST(SeededRng, DifferentSeeds) {
    seeded_rng a(1);
    seeded_rng b(2);
    bool any_differ = false;
    for (int i = 0; i < 100; ++i) {
        if (a.uniform() != b.uniform()) {
            any_differ = true;
            break;
        }
    }
    EXPECT_TRUE(any_differ);
}

// Derived RNGs for different modules are uncorrelated.
TEST(SeededRng, DeriveProducesDistinctStreams) {
    auto rng_a = seeded_rng::derive(42, 0);
    auto rng_b = seeded_rng::derive(42, 1);
    bool any_differ = false;
    for (int i = 0; i < 100; ++i) {
        if (rng_a.uniform() != rng_b.uniform()) {
            any_differ = true;
            break;
        }
    }
    EXPECT_TRUE(any_differ);
}

// Derive is deterministic: same master + module = same seed.
TEST(SeededRng, DeriveDeterministic) {
    auto a = seeded_rng::derive(99, 5);
    auto b = seeded_rng::derive(99, 5);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(a.uniform(), b.uniform());
    }
}

// Adding a new module (id=2) must not change the streams of existing
// modules (id=0 and id=1).
TEST(SeededRng, DeriveIndependentOfOtherModules) {
    auto before_0 = seeded_rng::derive(42, 0);
    auto before_1 = seeded_rng::derive(42, 1);

    // "add" module 2 (just derive it, doesn't affect 0 or 1)
    [[maybe_unused]] auto added = seeded_rng::derive(42, 2);

    auto after_0 = seeded_rng::derive(42, 0);
    auto after_1 = seeded_rng::derive(42, 1);

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(before_0.uniform(), after_0.uniform());
        EXPECT_EQ(before_1.uniform(), after_1.uniform());
    }
}

// Uniform samples should fall in [0, 1).
TEST(SeededRng, UniformRange) {
    seeded_rng rng(7);
    for (int i = 0; i < 100000; ++i) {
        double v = rng.uniform();
        EXPECT_GE(v, 0.0);
        EXPECT_LT(v, 1.0);
    }
}

TEST(SeededRng, UniformRangeScaled) {
    seeded_rng rng(8);
    for (int i = 0; i < 10000; ++i) {
        double v = rng.uniform(-5.0, 5.0);
        EXPECT_GE(v, -5.0);
        EXPECT_LT(v, 5.0);
    }
}

// Gaussian statistics: mean and stddev should converge over many samples.
TEST(SeededRng, GaussianStatistics) {
    seeded_rng rng(12345);
    constexpr int n = 500000;
    constexpr double target_mean = 3.0;
    constexpr double target_stddev = 2.0;

    double sum = 0.0;
    double sum_sq = 0.0;
    for (int i = 0; i < n; ++i) {
        double v = rng.gaussian(target_mean, target_stddev);
        sum += v;
        sum_sq += v * v;
    }

    double mean = sum / n;
    double variance = (sum_sq / n) - (mean * mean);
    double stddev = std::sqrt(variance);

    EXPECT_NEAR(mean, target_mean, 0.02);
    EXPECT_NEAR(stddev, target_stddev, 0.02);
}

// SplitMix64 is a bijection: distinct inputs produce distinct outputs.
TEST(SplitMix64, DistinctOutputs) {
    constexpr int n = 10000;
    std::vector<uint64_t> outputs(n);
    for (int i = 0; i < n; ++i) {
        outputs[static_cast<size_t>(i)] = splitmix64(static_cast<uint64_t>(i));
    }
    std::sort(outputs.begin(), outputs.end());
    auto it = std::adjacent_find(outputs.begin(), outputs.end());
    EXPECT_EQ(it, outputs.end());
}
