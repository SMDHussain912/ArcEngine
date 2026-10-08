add_executable(test_project test_project.cpp)
target_link_libraries(test_project PRIVATE ArcEngine)
set_target_properties(test_project PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_executable(test_asset_manager test_asset_manager.cpp)
target_link_libraries(test_asset_manager PRIVATE ArcEngine)
set_target_properties(test_asset_manager PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_executable(test_scene test_scene.cpp)
target_link_libraries(test_scene PRIVATE ArcEngine)
set_target_properties(test_scene PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_executable(test_sprites test_sprites.cpp)
target_link_libraries(test_sprites PRIVATE ArcEngine)
set_target_properties(test_sprites PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
