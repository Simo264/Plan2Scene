cmake_minimum_required(VERSION 3.22)
project(dbscan LANGUAGES C CXX)

add_library(simple_dbscan STATIC
  dbscan.h 
  kdtree.cpp
  kdtree.h
)
target_include_directories(simple_dbscan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(simple_dbscan::simple_dbscan ALIAS simple_dbscan)

message(STATUS "Created static target 'simple_dbscan' with sources: dbscan.h kdtree.cpp, kdtree.h")