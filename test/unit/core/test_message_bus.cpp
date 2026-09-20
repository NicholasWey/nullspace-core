#include "nullspace/core/message_bus.hpp"

#include <gtest/gtest.h>

#include <Eigen/Core>

using nullspace::core::message_bus;

namespace {

struct angular_velocity_msg {
    Eigen::Vector3d omega;
};

struct gyro_reading_msg {
    Eigen::Vector3d omega;
    double timestamp;
};

struct wheel_torque_cmd_msg {
    Eigen::Vector4d torques;
};

struct simple_msg {
    double value;
};

} // namespace

TEST(MessageBus, PublishAndRead) {
    message_bus bus;
    simple_msg msg{3.14};
    bus.publish(msg);

    auto result = bus.read<simple_msg>();
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->value, 3.14);
}

TEST(MessageBus, ReadMissingReturnsNullopt) {
    message_bus bus;
    auto result = bus.read<simple_msg>();
    EXPECT_FALSE(result.has_value());
}

TEST(MessageBus, PublishOverwritesPrevious) {
    message_bus bus;
    bus.publish(simple_msg{1.0});
    bus.publish(simple_msg{2.0});

    auto result = bus.read<simple_msg>();
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->value, 2.0);
}

TEST(MessageBus, DistinctTypesAreIndependent) {
    message_bus bus;

    angular_velocity_msg omega_msg;
    omega_msg.omega = Eigen::Vector3d(0.1, 0.2, 0.3);
    bus.publish(omega_msg);

    gyro_reading_msg gyro_msg;
    gyro_msg.omega = Eigen::Vector3d(0.11, 0.19, 0.31);
    gyro_msg.timestamp = 100.0;
    bus.publish(gyro_msg);

    EXPECT_EQ(bus.size(), 2u);

    auto omega = bus.read<angular_velocity_msg>();
    auto gyro = bus.read<gyro_reading_msg>();
    ASSERT_TRUE(omega.has_value());
    ASSERT_TRUE(gyro.has_value());

    EXPECT_DOUBLE_EQ(omega->omega.x(), 0.1);
    EXPECT_DOUBLE_EQ(gyro->omega.x(), 0.11);
    EXPECT_DOUBLE_EQ(gyro->timestamp, 100.0);
}

// The whole point: FSW reads estimated state, never truth.
// Both are Vector3d, but they're different types on the bus.
TEST(MessageBus, TruthAndEstimateSeparated) {
    struct true_attitude_msg { Eigen::Vector3d omega; };
    struct estimated_attitude_msg { Eigen::Vector3d omega; };

    message_bus bus;

    true_attitude_msg truth;
    truth.omega = Eigen::Vector3d(1.0, 2.0, 3.0);
    bus.publish(truth);

    estimated_attitude_msg estimate;
    estimate.omega = Eigen::Vector3d(1.01, 1.98, 3.02);
    bus.publish(estimate);

    auto t = bus.read<true_attitude_msg>();
    auto e = bus.read<estimated_attitude_msg>();
    ASSERT_TRUE(t.has_value());
    ASSERT_TRUE(e.has_value());
    EXPECT_NE(t->omega.x(), e->omega.x());
}

TEST(MessageBus, Has) {
    message_bus bus;
    EXPECT_FALSE(bus.has<simple_msg>());
    bus.publish(simple_msg{1.0});
    EXPECT_TRUE(bus.has<simple_msg>());
}

TEST(MessageBus, ClearSingleType) {
    message_bus bus;
    bus.publish(simple_msg{1.0});

    angular_velocity_msg omega_msg;
    omega_msg.omega = Eigen::Vector3d::Zero();
    bus.publish(omega_msg);

    EXPECT_EQ(bus.size(), 2u);
    bus.clear<simple_msg>();
    EXPECT_EQ(bus.size(), 1u);
    EXPECT_FALSE(bus.has<simple_msg>());
    EXPECT_TRUE(bus.has<angular_velocity_msg>());
}

TEST(MessageBus, ClearAll) {
    message_bus bus;
    bus.publish(simple_msg{1.0});

    angular_velocity_msg omega_msg;
    omega_msg.omega = Eigen::Vector3d::Zero();
    bus.publish(omega_msg);

    EXPECT_EQ(bus.size(), 2u);
    bus.clear_all();
    EXPECT_EQ(bus.size(), 0u);
}

TEST(MessageBus, WheelTorqueCommand) {
    message_bus bus;
    wheel_torque_cmd_msg cmd;
    cmd.torques = Eigen::Vector4d(0.01, -0.02, 0.015, -0.005);
    bus.publish(cmd);

    auto result = bus.read<wheel_torque_cmd_msg>();
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->torques[0], 0.01);
    EXPECT_DOUBLE_EQ(result->torques[3], -0.005);
}
