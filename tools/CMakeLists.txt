add_executable(nullspace-run nullspace_run.cpp)

target_link_libraries(nullspace-run
    PRIVATE nullspace_core
    PRIVATE nlohmann_json::nlohmann_json
)

target_link_libraries(nullspace-run PRIVATE nullspace_warnings)

set_target_properties(nullspace-run PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
