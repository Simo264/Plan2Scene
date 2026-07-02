#pragma once

#include "../types.hpp"
#include <vector>
#include <filesystem>


void export_to_gltf(const struct ReconstructionResult& result,
                    const std::filesystem::path& filename);