#include "nullspace/core/telemetry_logger.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using nullspace::core::telemetry_logger;

namespace {

std::filesystem::path test_dir() {
    auto dir = std::filesystem::temp_directory_path() / "nullspace_test_telem";
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

TEST(TelemetryLogger, AddChannelAndQuery) {
    telemetry_logger logger(0.0, 0.001, 42);

    size_t id0 = logger.add_channel("scalar", 1);
    size_t id1 = logger.add_channel("vector3", 3);

    EXPECT_EQ(id0, 0u);
    EXPECT_EQ(id1, 1u);
    EXPECT_EQ(logger.channel_count(), 2u);
    EXPECT_EQ(logger.channel_name(0), "scalar");
    EXPECT_EQ(logger.channel_name(1), "vector3");
    EXPECT_EQ(logger.channel_width(0), 1u);
    EXPECT_EQ(logger.channel_width(1), 3u);
}

TEST(TelemetryLogger, SetScalarAndCommit) {
    telemetry_logger logger(0.0, 0.001, 42);
    logger.add_channel("value", 1);

    logger.set(size_t{0}, 3.14);
    logger.commit(0.001);
    logger.set(size_t{0}, 2.72);
    logger.commit(0.002);

    EXPECT_EQ(logger.row_count(), 2u);
}

TEST(TelemetryLogger, SetVectorAndCommit) {
    telemetry_logger logger(0.0, 0.001, 42);
    logger.add_channel("omega", 3);

    double omega[] = {0.1, 0.2, 0.3};
    logger.set(size_t{0}, omega);
    logger.commit(0.001);

    EXPECT_EQ(logger.row_count(), 1u);
}

TEST(TelemetryLogger, MultipleChannels) {
    telemetry_logger logger(0.0, 0.001, 42);
    logger.add_channel("q", 4);
    logger.add_channel("omega", 3);
    logger.add_channel("energy", 1);

    double q[] = {1.0, 0.0, 0.0, 0.0};
    double omega[] = {0.01, 0.02, 0.03};
    logger.set(size_t{0}, q);
    logger.set(size_t{1}, omega);
    logger.set(size_t{2}, 42.0);
    logger.commit(0.001);

    EXPECT_EQ(logger.row_count(), 1u);
    EXPECT_EQ(logger.channel_count(), 3u);
}

TEST(TelemetryLogger, EmptyNoChannels) {
    telemetry_logger logger(0.0, 0.001, 42);
    logger.commit(0.001);
    logger.commit(0.002);
    logger.commit(0.003);

    EXPECT_EQ(logger.channel_count(), 0u);
    EXPECT_EQ(logger.row_count(), 3u);
}

TEST(TelemetryLogger, WriteBinaryCreatesFile) {
    telemetry_logger logger(100.0, 0.001, 99);
    logger.add_channel("x", 1);
    logger.set(size_t{0}, 1.0);
    logger.commit(100.001);
    logger.set(size_t{0}, 2.0);
    logger.commit(100.002);

    auto path = test_dir() / "test_write.bin";
    logger.write_binary(path);

    ASSERT_TRUE(std::filesystem::exists(path));
    auto file_size = std::filesystem::file_size(path);
    // Header: 4 (magic) + 4 (version) + 8 (epoch) + 8 (dt) + 8 (seed)
    //       + 4 (num_channels) + 8 (num_rows) = 44
    // Channel descriptor: 2 (name_len) + 1 (name "x") + 4 (width) = 7
    // Data: 2 rows * 2 doubles * 8 bytes = 32
    // Total: 44 + 7 + 32 = 83
    EXPECT_EQ(file_size, 83u);

    std::filesystem::remove_all(test_dir());
}

TEST(TelemetryLogger, BinaryRoundTrip) {
    telemetry_logger logger(50.0, 0.01, 7);
    logger.add_channel("pos", 3);
    logger.add_channel("speed", 1);

    double pos1[] = {1.0, 2.0, 3.0};
    logger.set(size_t{0}, pos1);
    logger.set(size_t{1}, 10.0);
    logger.commit(50.01);

    double pos2[] = {4.0, 5.0, 6.0};
    logger.set(size_t{0}, pos2);
    logger.set(size_t{1}, 20.0);
    logger.commit(50.02);

    auto path = test_dir() / "roundtrip.bin";
    logger.write_binary(path);

    // Read back and verify header
    std::ifstream in(path, std::ios::binary);
    ASSERT_TRUE(in.good());

    char magic[4];
    in.read(magic, 4);
    EXPECT_EQ(std::string(magic, 4), "NSTL");

    uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), 4);
    EXPECT_EQ(version, 1u);

    double epoch = 0.0;
    in.read(reinterpret_cast<char*>(&epoch), 8);
    EXPECT_DOUBLE_EQ(epoch, 50.0);

    double dt = 0.0;
    in.read(reinterpret_cast<char*>(&dt), 8);
    EXPECT_DOUBLE_EQ(dt, 0.01);

    uint64_t seed = 0;
    in.read(reinterpret_cast<char*>(&seed), 8);
    EXPECT_EQ(seed, 7u);

    uint32_t num_channels = 0;
    in.read(reinterpret_cast<char*>(&num_channels), 4);
    EXPECT_EQ(num_channels, 2u);

    uint64_t num_rows = 0;
    in.read(reinterpret_cast<char*>(&num_rows), 8);
    EXPECT_EQ(num_rows, 2u);

    // Channel descriptors
    uint16_t name_len = 0;
    char name_buf[64];

    in.read(reinterpret_cast<char*>(&name_len), 2);
    EXPECT_EQ(name_len, 3u);
    in.read(name_buf, name_len);
    EXPECT_EQ(std::string(name_buf, name_len), "pos");

    uint32_t width = 0;
    in.read(reinterpret_cast<char*>(&width), 4);
    EXPECT_EQ(width, 3u);

    in.read(reinterpret_cast<char*>(&name_len), 2);
    EXPECT_EQ(name_len, 5u);
    in.read(name_buf, name_len);
    EXPECT_EQ(std::string(name_buf, name_len), "speed");
    in.read(reinterpret_cast<char*>(&width), 4);
    EXPECT_EQ(width, 1u);

    // Data: row 0 = [50.01, 1, 2, 3, 10], row 1 = [50.02, 4, 5, 6, 20]
    double row[5];
    in.read(reinterpret_cast<char*>(row), 5 * 8);
    EXPECT_DOUBLE_EQ(row[0], 50.01);
    EXPECT_DOUBLE_EQ(row[1], 1.0);
    EXPECT_DOUBLE_EQ(row[2], 2.0);
    EXPECT_DOUBLE_EQ(row[3], 3.0);
    EXPECT_DOUBLE_EQ(row[4], 10.0);

    in.read(reinterpret_cast<char*>(row), 5 * 8);
    EXPECT_DOUBLE_EQ(row[0], 50.02);
    EXPECT_DOUBLE_EQ(row[1], 4.0);
    EXPECT_DOUBLE_EQ(row[2], 5.0);
    EXPECT_DOUBLE_EQ(row[3], 6.0);
    EXPECT_DOUBLE_EQ(row[4], 20.0);

    in.close();
    std::filesystem::remove_all(test_dir());
}

TEST(TelemetryLogger, WriteCSVContent) {
    telemetry_logger logger(0.0, 0.001, 42);
    logger.add_channel("x", 1);
    logger.add_channel("vel", 2);

    logger.set(size_t{0}, 1.5);
    double vel[] = {3.0, 4.0};
    logger.set(size_t{1}, vel);
    logger.commit(0.001);

    auto path = test_dir() / "test_write.csv";
    logger.write_csv(path);

    std::ifstream in(path);
    ASSERT_TRUE(in.good());

    std::string header;
    std::getline(in, header);
    EXPECT_EQ(header, "time,x,vel_0,vel_1");

    std::string row;
    std::getline(in, row);
    // Verify the row contains the expected values (exact format depends on precision)
    EXPECT_NE(row.find("0.001"), std::string::npos);
    EXPECT_NE(row.find("1.5"), std::string::npos);

    in.close();
    std::filesystem::remove_all(test_dir());
}

TEST(TelemetryLogger, OutOfRangeThrows) {
    telemetry_logger logger(0.0, 0.001, 42);
    EXPECT_THROW(logger.set(size_t{0}, 1.0), std::out_of_range);
    EXPECT_THROW(logger.channel_name(0), std::out_of_range);
    EXPECT_THROW(logger.channel_width(0), std::out_of_range);
}
