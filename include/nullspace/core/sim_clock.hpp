#pragma once

#include <cstdint>

namespace nullspace::core {

class sim_clock {
public:
    sim_clock(double epoch_tdb, double dt);

    void tick();
    void reset();

    double time() const;
    double dt() const;
    double epoch() const;
    double elapsed() const;
    uint64_t step_count() const;

private:
    double epoch_;
    double dt_;
    uint64_t step_count_ = 0;
};

} // namespace nullspace::core
