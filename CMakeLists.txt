cmake_minimum_required(VERSION 3.20)

project(nullspace-core
    VERSION     0.1.0
    LANGUAGES   CXX C
    DESCRIPTION "Hyper-realistic spacecraft GNC simulation engine"
)

# ── C++ standard ─────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build options ────────────────────────────────────────────────
option(NULLSPACE_BUILD_TESTS      "Build the test suite"            ON)
option(NULLSPACE_BUILD_BENCHMARKS "Build performance benchmarks"    ON)
option(NULLSPACE_BUILD_TOOLS      "Build CLI tools (headless runner, MC)" ON)

# ── Compiler warnings ───────────────────────────────────────────
add_library(nullspace_warnings INTERFACE)

if(MSVC)
    target_compile_options(nullspace_warnings INTERFACE
        /W4
        /permissive-
    )
else()
    target_compile_options(nullspace_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wno-unused-parameter
    )
endif()

# ── Dependencies (FetchContent) ─────────────────────────────────
include(cmake/dependencies.cmake)

# ── Library target ──────────────────────────────────────────────
add_subdirectory(src)

# ── Tests ───────────────────────────────────────────────────────
if(NULLSPACE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# ── CLI tools ───────────────────────────────────────────────────
if(NULLSPACE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
