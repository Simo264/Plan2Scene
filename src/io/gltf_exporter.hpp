#pragma once

#include "../geometry.hpp"
#include <vector>
#include <filesystem>

void export_to_gltf(const std::vector<Vertex>& vertices, 
                    const std::filesystem::path& filename);