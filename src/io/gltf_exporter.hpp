#pragma once

#include "../types.hpp"
#include <vector>
#include <filesystem>

void export_to_gltf(const std::vector<Vertex_PNT>& vertices, 
                    const std::vector<u32>& indices,
                    const std::filesystem::path& filename);