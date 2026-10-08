cmake_minimum_required(VERSION 3.18)
project(ArcEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(ARC_BUILD_EXAMPLES "Build examples" ON)
option(ARC_BUILD_TESTS "Build headless tests" ON)

add_subdirectory(engine)
add_subdirectory(runtime)

if(ARC_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(ARC_BUILD_TESTS)
  add_subdirectory(tests)
endif()

