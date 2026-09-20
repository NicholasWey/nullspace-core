add_executable(nullspace_tests
    core/test_sim_clock.cpp
    core/test_seeded_rng.cpp
    core/test_module.cpp
    core/test_message_bus.cpp
    core/test_scheduler.cpp
    core/test_telemetry_logger.cpp
)

target_link_libraries(nullspace_tests
    PRIVATE nullspace_core
    PRIVATE GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(nullspace_tests)
