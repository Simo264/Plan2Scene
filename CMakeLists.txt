cmake_minimum_required(VERSION 3.12)
project(Plan2Scene)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# --- Corrade ---
set(CORRADE_WITH_MAIN ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  corrade
  GIT_REPOSITORY https://github.com/mosra/corrade.git
  GIT_TAG master
  GIT_PROGRESS TRUE
)

# --- Magnum ---
set(MAGNUM_WITH_SDL2APPLICATION ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_GL ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_PRIMITIVES ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_MESHTOOLS ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_SHADERS ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_TRADE ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  magnum
  GIT_REPOSITORY https://github.com/mosra/magnum.git
  GIT_TAG master
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(corrade magnum)


add_executable(${PROJECT_NAME} 
  src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Corrade::Main
  Magnum::Sdl2Application
  Magnum::GL
  Magnum::Magnum
  Magnum::MeshTools
  Magnum::Primitives
  Magnum::Shaders
  Magnum::Trade
)
