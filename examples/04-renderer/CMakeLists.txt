add_executable(04-renderer main.cpp)
target_link_libraries(04-renderer PRIVATE ArcEngine)
set_target_properties(04-renderer PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/04-renderer
)
