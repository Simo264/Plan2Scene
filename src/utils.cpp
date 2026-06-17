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
    file << seg.start.x << "," << seg.start.y << "," << seg.end.x << "," << seg.end.y << "\n";
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

void dump_faces(const Arrangement& arr, std::string_view filename) 
{
  std::ofstream file(filename.data());
  if (!file.is_open()) 
  {
    std::println(stderr, "Errore: impossibile aprire il file {}", filename);
    return;
  }

  file << std::fixed << std::setprecision(6);
  for (auto fit = arr.faces_begin(); fit != arr.faces_end(); ++fit) 
  {
    if (fit->is_unbounded() || !fit->has_outer_ccb()) continue;

    auto curr = fit->outer_ccb();
    auto first = curr;
    do {
        auto p1 = curr->source()->point();
        auto p2 = curr->target()->point();
        file << CGAL::to_double(p1.x()) << "," << CGAL::to_double(p1.y()) << ","
              << CGAL::to_double(p2.x()) << "," << CGAL::to_double(p2.y()) << "\n";

        ++curr;
    } while (curr != first);
  }
}