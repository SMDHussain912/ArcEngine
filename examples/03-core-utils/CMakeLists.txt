add_executable(03-core-utils main.cpp)
target_link_libraries(03-core-utils PRIVATE ArcEngine)
set_target_properties(03-core-utils PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/03-core-utils
)
