#pragma once

#include "nullspace/core/seeded_rng.hpp"

#include <cstdint>
#include <string>

namespace nullspace::core {

class module {
public:
    module(std::string name, uint64_t id, uint64_t master_seed);
    virtual ~module() = default;

    module(const module&) = delete;
    module& operator=(const module&) = delete;

    virtual void init() = 0;
    virtual void update(double dt) = 0;
    virtual void cleanup() = 0;

    const std::string& name() const;
    uint64_t id() const;

protected:
    seeded_rng& rng();

private:
    std::string name_;
    uint64_t id_;
    seeded_rng rng_;
};

} // namespace nullspace::core
