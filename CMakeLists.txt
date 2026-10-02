cmake_minimum_required(VERSION 3.20)
project(efim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Tuned for the machine it is built on (Apple M-series).
option(EFIM_NATIVE "Compile with -mcpu=native" ON)
option(EFIM_SANITIZE "Build with ASan/UBSan" OFF)
option(EFIM_LTO "Enable link-time optimization" OFF)

find_package(Threads REQUIRED)
# Dependencies are downloaded at configure time (a system install is used if found
# and EFIM_FETCH_DEPS is OFF).
option(EFIM_FETCH_DEPS "Download spdlog and CLI11 with FetchContent" ON)
include(FetchContent)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CLI11_PRECOMPILED OFF CACHE BOOL "" FORCE)
if(EFIM_FETCH_DEPS)
  FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git
                       GIT_TAG v1.17.0 GIT_SHALLOW ON)
  FetchContent_Declare(CLI11 GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
                       GIT_TAG v2.7.2 GIT_SHALLOW ON)
  FetchContent_MakeAvailable(spdlog CLI11)
else()
  find_package(spdlog REQUIRED)
  find_package(CLI11 REQUIRED)
endif()

# Everything except main.cpp is a library so tests/benchmarks can link it.
add_library(efim_core
  src/core/level.cpp
  src/io/dataset.cpp
  src/io/pattern_writer.cpp
  src/mine/miner.cpp
  src/mine/worker.cpp
  src/util/sys.cpp)
target_include_directories(efim_core PUBLIC include)
target_link_libraries(efim_core PUBLIC spdlog::spdlog Threads::Threads)
target_compile_options(efim_core PUBLIC -Wall -Wextra)

add_executable(efim src/main.cpp)
target_link_libraries(efim PRIVATE efim_core CLI11::CLI11)

if(EFIM_NATIVE)
  target_compile_options(efim_core PUBLIC -mcpu=native)
endif()
if(EFIM_SANITIZE)
  target_compile_options(efim_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(efim_core PUBLIC -fsanitize=address,undefined)
endif()
if(EFIM_LTO)
  set_target_properties(efim_core efim PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

enable_testing()
add_test(NAME example
  COMMAND ${CMAKE_COMMAND}
    -DEFIM=$<TARGET_FILE:efim>
    -DDIR=${CMAKE_CURRENT_SOURCE_DIR}/tests
    -DOUT=${CMAKE_CURRENT_BINARY_DIR}/example.out
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_example.cmake)
