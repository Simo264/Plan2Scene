cmake_minimum_required(VERSION 3.22)
project(Plan2Scene)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# ---- libdxfrw -----
FetchContent_Declare(
  libdxfrw
  GIT_REPOSITORY https://github.com/LibreCAD/libdxfrw.git
  GIT_TAG        master
  GIT_PROGRESS   TRUE
)

## --- poly2tri ---
FetchContent_Declare(
  poly2tri
  GIT_REPOSITORY https://github.com/jhasse/poly2tri.git
  GIT_TAG        master
  GIT_PROGRESS   TRUE
)

# --- GLFW 3.4 ---
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable building GLFW docs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable building GLFW tests" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable building GLFW examples" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Disable GLFW install target" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)

# --- GLM ---
FetchContent_Declare(
 glm
 GIT_REPOSITORY	https://github.com/g-truc/glm.git
 GIT_TAG         0af55ccecd98d4e5a8d1fad7de25ba429d60e863
 GIT_SHALLOW     TRUE
 GIT_PROGRESS    TRUE
)

FetchContent_MakeAvailable(libdxfrw poly2tri glfw glm)

add_subdirectory("${CMAKE_SOURCE_DIR}/glad")
add_subdirectory("${CMAKE_SOURCE_DIR}/imgui")

add_executable(${PROJECT_NAME} 
  src/io/drw_parser.cpp

  src/graphics/buffer.cpp
  src/graphics/pipeline.cpp
  src/graphics/vertex_array.cpp
  src/graphics/camera.cpp
  src/graphics/static_mesh.cpp
  src/graphics/mesh_visualizer.cpp
  src/graphics/transformation.cpp

  src/main.cpp
  src/geometry.cpp
  src/mesh_builder.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  dxfrw
  poly2tri

  OpenGL::GL
  glad::glad
  glfw
  glm::glm
  imgui::imgui
)

target_include_directories(${PROJECT_NAME} PRIVATE ${libdxfrw_SOURCE_DIR}/src)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)