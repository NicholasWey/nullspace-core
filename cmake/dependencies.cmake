include(FetchContent)

# ── Eigen 3 ──────────────────────────────────────────────────────
# Header-only. We skip its CMakeLists.txt (legacy Fortran/Qt/BLAS
# detection) and create our own interface target from the headers.
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
    GIT_SHALLOW    TRUE
)
# Eigen is header-only. We use FetchContent_Populate to skip its legacy
# CMakeLists.txt (Fortran, Qt, BLAS detection). CMP0169 (CMake 3.30+)
# deprecates Populate; set it OLD to suppress that warning.
if(POLICY CMP0169)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0169 OLD)
endif()
FetchContent_GetProperties(eigen)
if(NOT eigen_POPULATED)
    FetchContent_Populate(eigen)
endif()
if(POLICY CMP0169)
    cmake_policy(POP)
endif()
add_library(eigen_headers INTERFACE)
target_include_directories(eigen_headers SYSTEM INTERFACE "${eigen_SOURCE_DIR}")
add_library(Eigen3::Eigen ALIAS eigen_headers)

# ── Google Test ──────────────────────────────────────────────────
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)

# ── Google Benchmark ─────────────────────────────────────────────
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.1
    GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

# ── nlohmann/json ────────────────────────────────────────────────
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest googlebenchmark json)

# CSPICE - JPL ephemeris library (Phase 3+)
# Not needed for Phase 1 (two-body + J2 only).
# CSPICE has no native CMake build. Integration options:
#   1. FetchContent from a GitHub mirror + custom CMakeLists
#   2. ExternalProject_Add with NAIF's platform-specific tarballs
# Will be resolved when multi-body gravity comes online.
