#pragma once

#include "spatial_hashing.hpp"

#include <vector>

// vertices: new vertices get added at split points
// edges: the split WALL edge is removed and replaced by two shorter edges
void resolve_tjunctions(std::vector<GraphVertex>& vertices,
                        std::vector<GraphEdge>& edges);