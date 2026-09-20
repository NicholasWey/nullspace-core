#pragma once

#include "nullspace/core/seeded_rng.hpp"

#include <cstdint>
#include <string>

namespace nullspace::core {

class message_bus;
class sim_clock;

class module {
public:
    module(std::string name, uint64_t id, uint64_t master_seed);
    virtual ~module() = default;

    module(const module&) = delete;
    module& operator=(const module&) = delete;

    virtual void init() = 0;
    virtual void update(double dt) = 0;
    virtual void cleanup() = 0;

    void attach(message_bus& bus, const sim_clock& clock);

    const std::string& name() const;
    uint64_t id() const;

protected:
    seeded_rng& rng();
    message_bus& bus();
    const sim_clock& clock() const;

private:
    std::string name_;
    uint64_t id_;
    seeded_rng rng_;
    message_bus* bus_ = nullptr;
    const sim_clock* clock_ = nullptr;
};

} // namespace nullspace::core
