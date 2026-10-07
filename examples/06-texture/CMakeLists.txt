add_executable(06-texture main.cpp)
target_link_libraries(06-texture PRIVATE ArcEngine)
set_target_properties(06-texture PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/06-texture
)
