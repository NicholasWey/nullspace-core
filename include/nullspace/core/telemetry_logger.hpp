#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace nullspace::core {

class telemetry_logger {
public:
    telemetry_logger(double epoch_tdb, double dt, uint64_t seed);

    size_t add_channel(const std::string& name, uint32_t width);

    void set(size_t channel_id, const double* values);
    void set(size_t channel_id, double value);

    void commit(double time);

    void write_binary(const std::filesystem::path& path) const;
    void write_csv(const std::filesystem::path& path) const;

    size_t channel_count() const;
    size_t row_count() const;
    const std::string& channel_name(size_t id) const;
    uint32_t channel_width(size_t id) const;

private:
    struct channel_info {
        std::string name;
        uint32_t width;
        size_t offset;
    };

    double epoch_;
    double dt_;
    uint64_t seed_;
    std::vector<channel_info> channels_;
    size_t row_width_ = 0;
    std::vector<double> current_;
    std::vector<double> data_;
};

} // namespace nullspace::core
