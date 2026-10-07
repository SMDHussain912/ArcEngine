add_executable(02-triangle main.cpp)
target_link_libraries(02-triangle PRIVATE ArcEngine)
set_target_properties(02-triangle PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/02-triangle
)
