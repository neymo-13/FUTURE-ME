cmake_minimum_required(VERSION 3.20)
project(OpenGLStarter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glew CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Generate an absolute shader directory for this clone. The program can then
# find shaders regardless of the working directory chosen by VS Code/VS/CMake.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ProjectPaths.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ProjectPaths.h
  @ONLY
)

add_executable(OpenGLStarter
  src/main.cpp
  src/Libs/Mesh.cpp
  src/Libs/Shader.cpp
  src/Libs/Window.cpp
)

target_include_directories(OpenGLStarter PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(OpenGLStarter PRIVATE
  OpenGL::GL
  glfw
  GLEW::GLEW
  glm::glm
)

if(APPLE)
  # OpenGL is deprecated by Apple but remains available (up to OpenGL 4.1).
  target_compile_definitions(OpenGLStarter PRIVATE GL_SILENCE_DEPRECATION)
endif()
