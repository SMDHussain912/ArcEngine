add_library(ArcEngine STATIC
  src/Core/Version.cpp
)

target_include_directories(ArcEngine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(ArcEngine PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)
