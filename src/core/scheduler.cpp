#include "nullspace/core/scheduler.hpp"
#include "nullspace/core/telemetry_logger.hpp"

#include <cmath>
#include <utility>

namespace nullspace::core {

scheduler::scheduler(double epoch_tdb, double base_dt, uint64_t master_seed)
    : clock_(epoch_tdb, base_dt)
    , master_seed_(master_seed) {}

void scheduler::attach_logger(telemetry_logger& logger) {
    logger_ = &logger;
}

void scheduler::add_module(std::unique_ptr<module> mod, uint64_t rate_divisor) {
    mod->attach(bus_, clock_);
    modules_.push_back({std::move(mod), rate_divisor});
}

void scheduler::init() {
    for (auto& entry : modules_) {
        entry.mod->init();
    }
}

// One base tick: advance clock, then update every module whose
// rate divisor divides the current step count evenly.
// Registration order is dispatch order.
void scheduler::step() {
    clock_.tick();
    uint64_t step = clock_.step_count();
    for (auto& entry : modules_) {
        if (step % entry.rate_divisor == 0) {
            double module_dt = clock_.dt() * static_cast<double>(entry.rate_divisor);
            entry.mod->update(module_dt);
        }
    }
    if (logger_) {
        logger_->commit(clock_.time());
    }
}

void scheduler::run(double duration_seconds) {
    auto total_steps = static_cast<uint64_t>(
        std::round(duration_seconds / clock_.dt()));
    for (uint64_t i = 0; i < total_steps; ++i) {
        step();
    }
}

void scheduler::cleanup() {
    for (auto& entry : modules_) {
        entry.mod->cleanup();
    }
}

sim_clock& scheduler::clock() {
    return clock_;
}

const sim_clock& scheduler::clock() const {
    return clock_;
}

message_bus& scheduler::bus() {
    return bus_;
}

uint64_t scheduler::master_seed() const {
    return master_seed_;
}

size_t scheduler::module_count() const {
    return modules_.size();
}

} // namespace nullspace::core
