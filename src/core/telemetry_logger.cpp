#include "nullspace/core/telemetry_logger.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace nullspace::core {

telemetry_logger::telemetry_logger(double epoch_tdb, double dt, uint64_t seed)
    : epoch_(epoch_tdb)
    , dt_(dt)
    , seed_(seed) {}

size_t telemetry_logger::add_channel(const std::string& name, uint32_t width) {
    size_t id = channels_.size();
    channels_.push_back({name, width, row_width_});
    row_width_ += width;
    current_.resize(row_width_, 0.0);
    return id;
}

void telemetry_logger::set(size_t channel_id, const double* values) {
    const auto& ch = channels_.at(channel_id);
    std::copy_n(values, ch.width, current_.data() + ch.offset);
}

void telemetry_logger::set(size_t channel_id, double value) {
    const auto& ch = channels_.at(channel_id);
    current_[ch.offset] = value;
}

void telemetry_logger::commit(double time) {
    data_.push_back(time);
    data_.insert(data_.end(), current_.begin(), current_.end());
}

void telemetry_logger::write_binary(const std::filesystem::path& path) const {
    auto dir = path.parent_path();
    if (!dir.empty()) {
        std::filesystem::create_directories(dir);
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot open output file: " + path.string());
    }

    // Header: magic, version, epoch, dt, seed, num_channels, num_rows
    out.write("NSTL", 4);

    uint32_t version = 1;
    out.write(reinterpret_cast<const char*>(&version), 4);
    out.write(reinterpret_cast<const char*>(&epoch_), 8);
    out.write(reinterpret_cast<const char*>(&dt_), 8);
    out.write(reinterpret_cast<const char*>(&seed_), 8);

    auto num_channels = static_cast<uint32_t>(channels_.size());
    out.write(reinterpret_cast<const char*>(&num_channels), 4);

    uint64_t num_rows = row_count();
    out.write(reinterpret_cast<const char*>(&num_rows), 8);

    // Channel descriptors
    for (const auto& ch : channels_) {
        auto name_len = static_cast<uint16_t>(ch.name.size());
        out.write(reinterpret_cast<const char*>(&name_len), 2);
        out.write(ch.name.data(), static_cast<std::streamsize>(name_len));
        out.write(reinterpret_cast<const char*>(&ch.width), 4);
    }

    // Data rows: [time, ch0_data..., ch1_data..., ...] packed float64
    if (!data_.empty()) {
        out.write(reinterpret_cast<const char*>(data_.data()),
                  static_cast<std::streamsize>(data_.size() * sizeof(double)));
    }
}

void telemetry_logger::write_csv(const std::filesystem::path& path) const {
    auto dir = path.parent_path();
    if (!dir.empty()) {
        std::filesystem::create_directories(dir);
    }

    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("cannot open output file: " + path.string());
    }

    out << std::setprecision(17);

    // Header row
    out << "time";
    for (const auto& ch : channels_) {
        if (ch.width == 1) {
            out << "," << ch.name;
        } else {
            for (uint32_t i = 0; i < ch.width; ++i) {
                out << "," << ch.name << "_" << i;
            }
        }
    }
    out << "\n";

    // Data rows
    size_t stride = 1 + row_width_;
    size_t rows = row_count();
    for (size_t r = 0; r < rows; ++r) {
        const double* row = data_.data() + r * stride;
        out << row[0];
        for (size_t i = 0; i < row_width_; ++i) {
            out << "," << row[1 + i];
        }
        out << "\n";
    }
}

size_t telemetry_logger::channel_count() const {
    return channels_.size();
}

size_t telemetry_logger::row_count() const {
    size_t stride = 1 + row_width_;
    return data_.size() / stride;
}

const std::string& telemetry_logger::channel_name(size_t id) const {
    return channels_.at(id).name;
}

uint32_t telemetry_logger::channel_width(size_t id) const {
    return channels_.at(id).width;
}

} // namespace nullspace::core
