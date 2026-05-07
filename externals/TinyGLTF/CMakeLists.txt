cmake_minimum_required(VERSION 3.22)

project(tiny_gltf LANGUAGES C CXX)

add_library(tiny_gltf STATIC "tinygltf_impl.cpp")

target_include_directories(tiny_gltf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(tiny_gltf::tiny_gltf ALIAS tiny_gltf)