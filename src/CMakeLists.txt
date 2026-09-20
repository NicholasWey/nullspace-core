add_library(nullspace_core STATIC
    core/sim_clock.cpp
    core/seeded_rng.cpp
    core/module.cpp
)

target_include_directories(nullspace_core
    PUBLIC  ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(nullspace_core
    PUBLIC  Eigen3::Eigen
    PRIVATE nlohmann_json::nlohmann_json
)

target_link_libraries(nullspace_core PRIVATE nullspace_warnings)
