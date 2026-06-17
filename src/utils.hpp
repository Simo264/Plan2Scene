#pragma once

#include <vector>
#include <string_view>
#include <glm/ext/vector_double2.hpp>

#include "arrangement.hpp"

void dump_segments(const std::vector<struct Segment>& segments, std::string_view filename);
void dump_vertices(const std::vector<glm::dvec2>& vertices, std::string_view filename);
void dump_faces(const Arrangement& arr, std::string_view filename);