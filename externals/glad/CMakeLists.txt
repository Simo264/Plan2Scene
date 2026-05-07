cmake_minimum_required(VERSION 3.22)

project(glad_loader LANGUAGES C CXX)

set(GLAD_INCLUDE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(GLAD_HEADER       "${GLAD_INCLUDE_DIR}/glad/gl.h")
set(GLAD_KHR          "${GLAD_INCLUDE_DIR}/KHR/khrplatform.h")
set(GLAD_SRC          "${CMAKE_CURRENT_SOURCE_DIR}/src/gl.c")

add_library(glad STATIC "${GLAD_SRC}")
target_include_directories(glad PUBLIC "${GLAD_INCLUDE_DIR}")
set_target_properties(glad PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  LINKER_LANGUAGE C
)
message(STATUS "Created static target 'glad' from ${GLAD_SRC}")

add_library(glad::glad ALIAS glad)