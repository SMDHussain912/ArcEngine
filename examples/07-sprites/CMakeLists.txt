add_executable(07-sprites main.cpp)
target_link_libraries(07-sprites PRIVATE ArcEngine)
set_target_properties(07-sprites PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/07-sprites
)
