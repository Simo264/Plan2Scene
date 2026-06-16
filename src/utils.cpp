#include "utils.hpp"

#include "types.hpp"

#include <print>
#include <fstream>
#include <iomanip>

void dump_segments(const std::vector<Segment>& segments, std::string_view filename) 
{
  auto file = std::ofstream(filename.data());
    
  if (!file.is_open()) 
  {
    std::println("Error on opening file: {}", filename);
    return;
  }
  
  file << std::fixed << std::setprecision(6);
  for (const auto& seg : segments) 
    file << seg.p1.x << "," << seg.p1.y << "," << seg.p2.x << "," << seg.p2.y << "\n";
}

void dump_vertices(const std::vector<glm::dvec2>& vertices, std::string_view filename) 
{
  auto file = std::ofstream(filename.data());
  if (!file.is_open()) 
  {
    std::println("Error on opening file: {}", filename);
    return;
  }

  file << std::fixed << std::setprecision(6);
  for (const auto& v : vertices) 
    file << v.x << "," << v.y << "\n";
}
