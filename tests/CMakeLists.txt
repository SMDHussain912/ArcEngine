add_executable(test_project test_project.cpp)
target_link_libraries(test_project PRIVATE ArcEngine)
set_target_properties(test_project PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
