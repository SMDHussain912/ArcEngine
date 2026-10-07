add_library(glad STATIC
  src/gl.c
)

target_include_directories(glad PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(glad PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
)
