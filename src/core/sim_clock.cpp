#include "nullspace/core/sim_clock.hpp"

namespace nullspace::core {

sim_clock::sim_clock(double epoch_tdb, double dt)
    : epoch_(epoch_tdb), dt_(dt) {}

void sim_clock::tick() {
    ++step_count_;
}

void sim_clock::reset() {
    step_count_ = 0;
}

// Computed from integer step count, not accumulated.
// Error bounded by 2u * |result| regardless of step count.
double sim_clock::time() const {
    return epoch_ + static_cast<double>(step_count_) * dt_;
}

double sim_clock::dt() const {
    return dt_;
}

double sim_clock::epoch() const {
    return epoch_;
}

double sim_clock::elapsed() const {
    return static_cast<double>(step_count_) * dt_;
}

uint64_t sim_clock::step_count() const {
    return step_count_;
}

} // namespace nullspace::core
