#include "nullspace/core/scheduler.hpp"
#include "nullspace/core/telemetry_logger.hpp"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

using json = nlohmann::json;

namespace {

struct run_config {
    std::string name;
    double epoch_tdb = 0.0;
    double dt = 0.001;
    double duration = 10.0;
    uint64_t seed = 42;
    std::string output_format = "binary";
    std::filesystem::path output_dir = "output";
};

run_config load_scenario(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("cannot open config: " + path.string());
    }

    json j = json::parse(file);

    run_config cfg;
    cfg.name = j.value("name", path.stem().string());
    cfg.epoch_tdb = j.value("epoch_tdb", 0.0);
    cfg.dt = j.at("dt").get<double>();
    cfg.duration = j.at("duration").get<double>();
    cfg.seed = j.value("seed", static_cast<uint64_t>(42));

    if (j.contains("output")) {
        const auto& out = j["output"];
        cfg.output_format = out.value("format", std::string("binary"));
        cfg.output_dir = out.value("directory", std::string("output"));
    }

    return cfg;
}

void print_usage() {
    std::cerr << "Usage: nullspace-run --config <path.json> "
                 "[--seed N] [--format binary|csv] [--output <dir>]\n";
}

} // namespace

int main(int argc, char* argv[]) {
    std::filesystem::path config_path;
    std::optional<uint64_t> seed_override;
    std::optional<std::string> format_override;
    std::optional<std::filesystem::path> output_override;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--config") && i + 1 < argc) {
            config_path = argv[++i];
        } else if ((arg == "--seed") && i + 1 < argc) {
            try {
                seed_override = std::stoull(std::string(argv[++i]));
            } catch (const std::exception&) {
                std::cerr << "Error: invalid seed value\n";
                return 1;
            }
        } else if ((arg == "--format") && i + 1 < argc) {
            format_override = argv[++i];
        } else if ((arg == "--output") && i + 1 < argc) {
            output_override = argv[++i];
        } else {
            print_usage();
            return 1;
        }
    }

    if (config_path.empty()) {
        std::cerr << "Error: --config is required\n";
        print_usage();
        return 1;
    }

    try {
        run_config cfg = load_scenario(config_path);

        if (seed_override) cfg.seed = *seed_override;
        if (format_override) cfg.output_format = *format_override;
        if (output_override) cfg.output_dir = *output_override;

        auto total_steps = static_cast<uint64_t>(std::round(cfg.duration / cfg.dt));

        std::cout << "NullSpace Core v0.1.0\n"
                  << "Config:   " << config_path.string() << "\n"
                  << "Seed:     " << cfg.seed << "\n"
                  << "Epoch:    " << cfg.epoch_tdb << " TDB\n"
                  << "DT:       " << cfg.dt << " s ("
                  << static_cast<uint64_t>(1.0 / cfg.dt) << " Hz)\n"
                  << "Duration: " << cfg.duration << " s ("
                  << total_steps << " steps)\n";

        nullspace::core::scheduler sched(cfg.epoch_tdb, cfg.dt, cfg.seed);
        nullspace::core::telemetry_logger logger(cfg.epoch_tdb, cfg.dt, cfg.seed);
        sched.attach_logger(logger);

        std::cout << "Modules:  " << sched.module_count() << "\n\n";

        std::cout << "Running...\n";
        auto wall_start = std::chrono::steady_clock::now();

        sched.init();
        sched.run(cfg.duration);
        sched.cleanup();

        auto wall_end = std::chrono::steady_clock::now();
        double wall_s = std::chrono::duration<double>(wall_end - wall_start).count();

        std::cout << "Complete.\n"
                  << "Wall time:       " << wall_s << " s\n"
                  << "Sim time:        " << cfg.duration << " s\n";
        if (wall_s > 0.0) {
            std::cout << "Realtime factor: " << (cfg.duration / wall_s) << "x\n";
        }

        std::string filename = cfg.name + "_" + std::to_string(cfg.seed);
        std::filesystem::path output_path;

        if (cfg.output_format == "csv") {
            output_path = cfg.output_dir / (filename + ".csv");
            logger.write_csv(output_path);
        } else {
            output_path = cfg.output_dir / (filename + ".bin");
            logger.write_binary(output_path);
        }

        std::cout << "\nOutput: " << output_path.string()
                  << " (" << logger.channel_count() << " channels, "
                  << logger.row_count() << " rows)\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
