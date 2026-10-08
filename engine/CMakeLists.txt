add_library(ArcEngine STATIC
  src/Core/Version.cpp
  src/Core/Log.cpp
  src/Core/Time.cpp
  src/Core/Window.cpp
  src/Core/Input.cpp
  src/Core/File.cpp
  src/Core/App.cpp
  src/Renderer/GraphicsContext.cpp
  src/Renderer/Shader.cpp
  src/Renderer/Buffer.cpp
  src/Renderer/OpenGLRenderer.cpp
  src/Renderer/RenderCommand.cpp
  src/Renderer/Texture.cpp
  src/Audio/Audio.cpp
  src/Scene/Scene.cpp
  src/Scene/MeshComponent.cpp
)

target_include_directories(ArcEngine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# GLFW for windowing (M1). System package on Linux dev machine.
# OpenGL for glClear in M1 loop (M2 adds glad loader, still needs libGL).
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(OpenGL REQUIRED)

# glm 1.0.3 system package (M3a). Vendored copy comes before Android (M8).
find_package(glm REQUIRED)

# Vendored glad2 loader (M2): Desktop GL 4.6 Core.
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glad ${CMAKE_BINARY_DIR}/third_party/glad)

# Vendored stb headers (M6a): image loading, header-only.
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/stb ${CMAKE_BINARY_DIR}/third_party/stb)

# Vendored miniaudio (M6b): playback, single header. Needs pthread + m + dl on Linux.
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/miniaudio ${CMAKE_BINARY_DIR}/third_party/miniaudio)

target_include_directories(ArcEngine PUBLIC ${GLFW_INCLUDE_DIRS})
target_link_directories(ArcEngine PUBLIC ${GLFW_LIBRARY_DIRS})
target_link_libraries(ArcEngine PUBLIC ${GLFW_LIBRARIES} OpenGL::GL glad stb miniaudio glm::glm pthread m dl)
target_compile_options(ArcEngine PUBLIC ${GLFW_CFLAGS_OTHER})

set_target_properties(ArcEngine PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

