#pragma once

#include <any>
#include <optional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace nullspace::core {

class message_bus {
public:
    template <typename T>
    void publish(const T& msg) {
        slots_[std::type_index(typeid(T))] = msg;
    }

    template <typename T>
    std::optional<T> read() const {
        auto it = slots_.find(std::type_index(typeid(T)));
        if (it == slots_.end()) {
            return std::nullopt;
        }
        return std::any_cast<T>(it->second);
    }

    template <typename T>
    bool has() const {
        return slots_.contains(std::type_index(typeid(T)));
    }

    template <typename T>
    void clear() {
        slots_.erase(std::type_index(typeid(T)));
    }

    void clear_all() {
        slots_.clear();
    }

    size_t size() const {
        return slots_.size();
    }

private:
    std::unordered_map<std::type_index, std::any> slots_;
};

} // namespace nullspace::core
