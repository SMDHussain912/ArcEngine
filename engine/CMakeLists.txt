add_library(ArcEngine STATIC
  src/Core/Version.cpp
  src/Core/Log.cpp
  src/Core/Time.cpp
  src/Core/Window.cpp
  src/Core/Input.cpp
  src/Core/File.cpp
  src/Core/App.cpp
  src/Core/Project.cpp
  src/Assets/AssetManager.cpp
  src/Renderer/GraphicsContext.cpp
  src/Renderer/Shader.cpp
  src/Renderer/Buffer.cpp
  src/Renderer/OpenGLRenderer.cpp
  src/Renderer/RenderCommand.cpp
  src/Renderer/Texture.cpp
  src/Audio/Audio.cpp
  src/Scene/Scene.cpp
  src/Scene/SceneSerializer.cpp
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

# yaml-cpp (P0 Step 4): .arc scene serialization. System now; vendor pre-Android.
find_package(yaml-cpp REQUIRED)

# Vendored glad2 loader (M2): Desktop GL 4.6 Core.
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glad ${CMAKE_BINARY_DIR}/third_party/glad)

# Vendored stb headers (M6a): image loading, header-only.
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/stb ${CMAKE_BINARY_DIR}/third_party/stb)

# Vendored miniaudio (M6b): playback, single header. Needs pthread + m + dl on Linux.
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/miniaudio ${CMAKE_BINARY_DIR}/third_party/miniaudio)

# ImGui (M7 editor): OpenGL3 backend over our existing GLFW/GLAD context.
# Vendored copy tracks v1.92.9 (third_party/imgui/imgui-1.92.9).
# No CMake +config shipped (vendored copy is source-only), so list sources
# directly here instead of add_subdirectory.
target_sources(ArcEngine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/imgui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/imgui_widgets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/imgui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/imgui_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/backends/imgui_impl_glfw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/backends/imgui_impl_opengl3.cpp
)

target_include_directories(ArcEngine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/backends
)
target_link_libraries(ArcEngine PUBLIC ${GLFW_LIBRARIES} OpenGL::GL glad stb miniaudio glm::glm yaml-cpp pthread m dl)
target_compile_options(ArcEngine PUBLIC ${GLFW_CFLAGS_OTHER})

set_target_properties(ArcEngine PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

target_sources(ArcEngine PRIVATE
  src/Scene/MeshComponent.cpp
  src/Scene/Scene.cpp
  src/Scene/SceneSerializer.cpp
  src/Editor/ArcEditor.cpp
)

# ImGui source + backend files (vendored copy, M7), kept as plain sources.
target_sources(ArcEngine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/imgui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/imgui_widgets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/imgui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/imgui_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/backends/imgui_impl_glfw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui/imgui-1.92.9/backends/imgui_impl_opengl3.cpp
)

