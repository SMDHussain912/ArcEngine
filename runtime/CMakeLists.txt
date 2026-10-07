add_executable(ArcRuntime src/main.cpp)

target_link_libraries(ArcRuntime PRIVATE ArcEngine)

set_target_properties(ArcRuntime PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/runtime
)
