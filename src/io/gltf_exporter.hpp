#pragma once

#include <filesystem>

void export_to_gltf(const struct ReconstructionResult& result,
                    const std::filesystem::path& filename);

void export_opening_placeholders(const struct ReconstructionResult& result,
                                 const std::filesystem::path& filename);