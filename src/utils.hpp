#pragma once

#include <vector>
#include <string_view>
#include <glm/ext/vector_double2.hpp>

#include "types.hpp"

void dump_segments_csv(const std::vector<Segment>& segments, std::string_view filename);
void dump_vertices_csv(const std::vector<glm::dvec2>& vertices, std::string_view filename);
void dump_faces_csv(const std::vector<struct Face>& faces, std::string_view filename);

void dump_clusters_csv(const std::vector<glm::dvec2>& points,
                       const std::vector<std::vector<u32>>& clusters,
                       std::string_view filename);


