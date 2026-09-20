#include "nullspace/core/sim_clock.hpp"

#include <gtest/gtest.h>

#include <cmath>

using nullspace::core::sim_clock;

TEST(SimClock, InitialState) {
    sim_clock clk(100.0, 0.001);
    EXPECT_DOUBLE_EQ(clk.time(), 100.0);
    EXPECT_DOUBLE_EQ(clk.dt(), 0.001);
    EXPECT_DOUBLE_EQ(clk.epoch(), 100.0);
    EXPECT_DOUBLE_EQ(clk.elapsed(), 0.0);
    EXPECT_EQ(clk.step_count(), 0u);
}

TEST(SimClock, SingleTick) {
    sim_clock clk(0.0, 0.01);
    clk.tick();
    EXPECT_DOUBLE_EQ(clk.time(), 0.01);
    EXPECT_DOUBLE_EQ(clk.elapsed(), 0.01);
    EXPECT_EQ(clk.step_count(), 1u);
}

TEST(SimClock, TimeFromEpoch) {
    double epoch = 5000.0;
    sim_clock clk(epoch, 0.1);
    for (int i = 0; i < 100; ++i) {
        clk.tick();
    }
    EXPECT_DOUBLE_EQ(clk.time(), epoch + 10.0);
    EXPECT_DOUBLE_EQ(clk.elapsed(), 10.0);
}

TEST(SimClock, Reset) {
    sim_clock clk(0.0, 0.001);
    for (int i = 0; i < 1000; ++i) {
        clk.tick();
    }
    EXPECT_EQ(clk.step_count(), 1000u);

    clk.reset();
    EXPECT_EQ(clk.step_count(), 0u);
    EXPECT_DOUBLE_EQ(clk.time(), 0.0);
    EXPECT_DOUBLE_EQ(clk.elapsed(), 0.0);
}

// Verify that time = epoch + step_count * dt matches to machine precision
// even after many steps. This is the whole point of multiply-not-accumulate.
TEST(SimClock, PrecisionAfterManySteps) {
    double epoch = 730000000.0;  // ~23 years past J2000
    double dt = 0.001;           // 1 kHz
    uint64_t n = 86400000;       // 1 day of sim time

    sim_clock clk(epoch, dt);
    for (uint64_t i = 0; i < n; ++i) {
        clk.tick();
    }

    double expected = epoch + static_cast<double>(n) * dt;
    EXPECT_DOUBLE_EQ(clk.time(), expected);

    // Elapsed should be exactly n * dt, independent of epoch magnitude
    double expected_elapsed = static_cast<double>(n) * dt;
    EXPECT_DOUBLE_EQ(clk.elapsed(), expected_elapsed);
}
