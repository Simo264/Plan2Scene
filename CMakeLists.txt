cmake_minimum_required(VERSION 3.22)
project(Plan2Scene)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(poly2tri REQUIRED)
find_package(clipper2 REQUIRED)
find_package(cgal REQUIRED)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
# ---- libdxfrw -----
FetchContent_Declare(
  libdxfrw
  GIT_REPOSITORY https://github.com/LibreCAD/libdxfrw.git
  GIT_TAG        master
  GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(libdxfrw)

add_subdirectory("${CMAKE_SOURCE_DIR}/externals/glad")
add_subdirectory("${CMAKE_SOURCE_DIR}/externals/imgui")
add_subdirectory("${CMAKE_SOURCE_DIR}/externals/tinygltf/")
add_subdirectory("${CMAKE_SOURCE_DIR}/externals/SimpleDBSCAN/")

add_executable(${PROJECT_NAME} 
  src/io/drw_parser.cpp
  src/io/gltf_exporter.cpp

  src/graphics/buffer.cpp
  src/graphics/pipeline.cpp
  src/graphics/vertex_array.cpp
  src/graphics/camera.cpp
  src/graphics/static_mesh.cpp
  src/graphics/mesh_visualizer.cpp
  src/graphics/transformation.cpp

  src/main.cpp
  src/reconstruction.cpp
  src/utils.cpp
  src/geometry.cpp
  src/spatial_hashing.cpp
  src/arrangement.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  OpenGL::GL
  glad::glad
  glfw
  glm::glm

  dxfrw
  poly2tri::poly2tri
  clipper2::clipper2
  CGAL::CGAL

  imgui::imgui
  tiny_gltf::tiny_gltf
  simple_dbscan::simple_dbscan
)

target_include_directories(${PROJECT_NAME} PRIVATE ${libdxfrw_SOURCE_DIR}/src)

target_compile_options(${PROJECT_NAME} PRIVATE 
  -Wall 
  -Wextra 
  -Wpedantic
)