#include "nullspace/core/module.hpp"

#include <utility>

namespace nullspace::core {

module::module(std::string name, uint64_t id, uint64_t master_seed)
    : name_(std::move(name))
    , id_(id)
    , rng_(seeded_rng::derive(master_seed, id)) {}

const std::string& module::name() const {
    return name_;
}

uint64_t module::id() const {
    return id_;
}

seeded_rng& module::rng() {
    return rng_;
}

} // namespace nullspace::core
