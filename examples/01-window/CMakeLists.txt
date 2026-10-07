add_executable(01-window main.cpp)
target_link_libraries(01-window PRIVATE ArcEngine)
set_target_properties(01-window PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/01-window
)
