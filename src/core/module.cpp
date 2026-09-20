#include "nullspace/core/module.hpp"
#include "nullspace/core/message_bus.hpp"
#include "nullspace/core/sim_clock.hpp"

#include <stdexcept>
#include <utility>

namespace nullspace::core {

module::module(std::string name, uint64_t id, uint64_t master_seed)
    : name_(std::move(name))
    , id_(id)
    , rng_(seeded_rng::derive(master_seed, id)) {}

void module::attach(message_bus& bus, const sim_clock& clock) {
    bus_ = &bus;
    clock_ = &clock;
}

const std::string& module::name() const {
    return name_;
}

uint64_t module::id() const {
    return id_;
}

seeded_rng& module::rng() {
    return rng_;
}

message_bus& module::bus() {
    return *bus_;
}

const sim_clock& module::clock() const {
    return *clock_;
}

} // namespace nullspace::core
