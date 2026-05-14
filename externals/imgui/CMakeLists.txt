cmake_minimum_required(VERSION 3.22)

project(imgui LANGUAGES C CXX)

find_package(OpenGL QUIET)
find_package(glfw3 REQUIRED)

set(IMGUI_SOURCES
  imgui.cpp
  imgui_impl_opengl3.cpp
  imgui_demo.cpp
  imgui_stdlib.cpp
  imgui_draw.cpp
  imgui_tables.cpp
  imgui_impl_glfw.cpp
  imgui_widgets.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(imgui PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

target_compile_definitions(imgui PRIVATE 
  IMGUI_IMPL_OPENGL_LOADER_GLAD
  GLFW_EXPOSE_NATIVE_WAYLAND
  IMGUI_IMPL_GLFW_DISABLE_X11
)
target_include_directories(imgui PUBLIC "${CMAKE_SOURCE_DIR}/externals/glad/include")


target_link_libraries(imgui PUBLIC 
	OpenGL::GL 
	glfw 
	glad
)

add_library(imgui::imgui ALIAS imgui)
