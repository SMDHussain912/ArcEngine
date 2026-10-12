add_executable(08-physics main.cpp)
target_link_libraries(08-physics PRIVATE ArcEngine)
set_target_properties(08-physics PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/08-physics
)
