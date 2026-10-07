add_executable(05-scene main.cpp)
target_link_libraries(05-scene PRIVATE ArcEngine)
set_target_properties(05-scene PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/05-scene
)
