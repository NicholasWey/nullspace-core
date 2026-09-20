#include "nullspace/core/scheduler.hpp"

#include <gtest/gtest.h>

#include <Eigen/Core>
#include <vector>

using nullspace::core::message_bus;
using nullspace::core::module;
using nullspace::core::scheduler;

namespace {

struct counter_msg {
    int count;
};

struct producer_msg {
    double value;
};

// Counts how many times each lifecycle method is called.
class counting_module : public module {
public:
    using module::module;

    int init_count = 0;
    int update_count = 0;
    int cleanup_count = 0;

    void init() override { ++init_count; }
    void update(double dt) override { ++update_count; }
    void cleanup() override { ++cleanup_count; }
};

// Publishes its update count to the bus each tick.
class producer_module : public module {
public:
    using module::module;

    int count = 0;

    void init() override {}
    void update(double /*dt*/) override {
        ++count;
        bus().publish(counter_msg{count});
    }
    void cleanup() override {}
};

// Reads the producer's count from the bus and records it.
class consumer_module : public module {
public:
    using module::module;

    std::vector<int> observed;

    void init() override {}
    void update(double /*dt*/) override {
        auto msg = bus().read<counter_msg>();
        if (msg) {
            observed.push_back(msg->count);
        }
    }
    void cleanup() override {}
};

// Records the dt it receives each update.
class dt_recorder_module : public module {
public:
    using module::module;

    std::vector<double> received_dts;

    void init() override {}
    void update(double dt) override {
        received_dts.push_back(dt);
    }
    void cleanup() override {}
};

} // namespace

TEST(Scheduler, BasicLifecycle) {
    scheduler sched(0.0, 0.001, 42);
    auto mod = std::make_unique<counting_module>("test", 0, 42);
    auto* ptr = mod.get();
    sched.add_module(std::move(mod));

    sched.init();
    EXPECT_EQ(ptr->init_count, 1);

    sched.step();
    sched.step();
    sched.step();
    EXPECT_EQ(ptr->update_count, 3);

    sched.cleanup();
    EXPECT_EQ(ptr->cleanup_count, 1);
}

TEST(Scheduler, ClockAdvances) {
    scheduler sched(100.0, 0.001, 42);
    sched.step();
    sched.step();
    EXPECT_DOUBLE_EQ(sched.clock().time(), 100.002);
    EXPECT_EQ(sched.clock().step_count(), 2u);
}

TEST(Scheduler, RunDuration) {
    scheduler sched(0.0, 0.001, 42);
    auto mod = std::make_unique<counting_module>("test", 0, 42);
    auto* ptr = mod.get();
    sched.add_module(std::move(mod));

    sched.init();
    sched.run(1.0);  // 1 second at 1 kHz = 1000 steps
    EXPECT_EQ(ptr->update_count, 1000);
    EXPECT_DOUBLE_EQ(sched.clock().elapsed(), 1.0);
}

// Module at divisor 10 should run 1/10th as often.
TEST(Scheduler, MultiRate) {
    scheduler sched(0.0, 0.001, 42);

    auto fast = std::make_unique<counting_module>("fast", 0, 42);
    auto slow = std::make_unique<counting_module>("slow", 1, 42);
    auto* fast_ptr = fast.get();
    auto* slow_ptr = slow.get();

    sched.add_module(std::move(fast), 1);   // every tick (1 kHz)
    sched.add_module(std::move(slow), 10);  // every 10th tick (100 Hz)

    sched.init();
    sched.run(0.1);  // 100 base ticks

    EXPECT_EQ(fast_ptr->update_count, 100);
    EXPECT_EQ(slow_ptr->update_count, 10);
}

// Modules at different rates receive different dt values.
TEST(Scheduler, MultiRateDt) {
    scheduler sched(0.0, 0.001, 42);

    auto fast = std::make_unique<dt_recorder_module>("fast", 0, 42);
    auto slow = std::make_unique<dt_recorder_module>("slow", 1, 42);
    auto* fast_ptr = fast.get();
    auto* slow_ptr = slow.get();

    sched.add_module(std::move(fast), 1);    // dt = 0.001
    sched.add_module(std::move(slow), 10);   // dt = 0.01

    sched.init();
    sched.run(0.01);  // 10 base ticks

    EXPECT_EQ(fast_ptr->received_dts.size(), 10u);
    EXPECT_EQ(slow_ptr->received_dts.size(), 1u);
    EXPECT_DOUBLE_EQ(fast_ptr->received_dts[0], 0.001);
    EXPECT_DOUBLE_EQ(slow_ptr->received_dts[0], 0.01);
}

// Producer publishes, consumer reads from the same bus.
TEST(Scheduler, ModulesCommunicateViaBus) {
    scheduler sched(0.0, 0.001, 42);

    auto prod = std::make_unique<producer_module>("producer", 0, 42);
    auto cons = std::make_unique<consumer_module>("consumer", 1, 42);
    auto* cons_ptr = cons.get();

    // Producer runs first (registered first), consumer reads after.
    sched.add_module(std::move(prod));
    sched.add_module(std::move(cons));

    sched.init();
    sched.run(0.005);  // 5 ticks

    ASSERT_EQ(cons_ptr->observed.size(), 5u);
    EXPECT_EQ(cons_ptr->observed[0], 1);
    EXPECT_EQ(cons_ptr->observed[4], 5);
}

// Two runs with the same seed produce identical results.
TEST(Scheduler, DeterministicReplay) {
    auto run_sim = [](uint64_t seed) {
        scheduler sched(0.0, 0.001, seed);
        auto prod = std::make_unique<producer_module>("p", 0, seed);
        auto cons = std::make_unique<consumer_module>("c", 1, seed);
        auto* cons_ptr = cons.get();

        sched.add_module(std::move(prod));
        sched.add_module(std::move(cons));
        sched.init();
        sched.run(0.01);  // 10 ticks
        return cons_ptr->observed;
    };

    auto run1 = run_sim(42);
    auto run2 = run_sim(42);
    EXPECT_EQ(run1, run2);
}
