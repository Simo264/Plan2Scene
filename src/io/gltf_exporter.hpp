#pragma once

#include "../geometry.hpp"
#include <vector>
#include <filesystem>

void export_to_gltf(const std::vector<Vertex_PN>& vertices, 
                    const std::vector<u32>& indices,
                    const std::filesystem::path& filename);


void import_gltf(const std::filesystem::path& filename, 
                 std::vector<Vertex_PN>& out_vertices, 
                 std::vector<u32>& out_indices);