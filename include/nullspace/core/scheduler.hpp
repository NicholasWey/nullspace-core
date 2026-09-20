#pragma once

#include "nullspace/core/message_bus.hpp"
#include "nullspace/core/module.hpp"
#include "nullspace/core/sim_clock.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace nullspace::core {

class telemetry_logger;

class scheduler {
public:
    scheduler(double epoch_tdb, double base_dt, uint64_t master_seed);

    void attach_logger(telemetry_logger& logger);

    // Register a module. rate_divisor controls how often it runs:
    //   1 = every base tick (e.g., 1 kHz dynamics)
    //  10 = every 10th tick (e.g., 100 Hz FSW)
    // 100 = every 100th tick (e.g., 10 Hz guidance)
    void add_module(std::unique_ptr<module> mod, uint64_t rate_divisor = 1);

    void init();
    void step();
    void run(double duration_seconds);
    void cleanup();

    sim_clock& clock();
    const sim_clock& clock() const;
    message_bus& bus();
    uint64_t master_seed() const;
    size_t module_count() const;

private:
    struct module_entry {
        std::unique_ptr<module> mod;
        uint64_t rate_divisor;
    };

    sim_clock clock_;
    message_bus bus_;
    uint64_t master_seed_;
    telemetry_logger* logger_ = nullptr;
    std::vector<module_entry> modules_;
};

} // namespace nullspace::core
