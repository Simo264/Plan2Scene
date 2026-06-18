#include "utils.hpp"

#include "types.hpp"

#include <print>
#include <fstream>
#include <iomanip>

void dump_segments_csv(const std::vector<Segment>& segments, std::string_view filename) 
{
  auto file = std::ofstream(filename.data());
  file << std::fixed << std::setprecision(6);
  for (const auto& seg : segments) 
    file << seg.start.x << "," << seg.start.y << "," << seg.end.x << "," << seg.end.y << "\n";
}

void dump_vertices_csv(const std::vector<glm::dvec2>& vertices, std::string_view filename) 
{
  auto file = std::ofstream(filename.data());
  file << std::fixed << std::setprecision(6);
  for (const auto& v : vertices) 
    file << v.x << "," << v.y << "\n";
}

void dump_faces_csv(const Arrangement& arr, std::string_view filename) 
{
  auto file = std::ofstream (filename.data());
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

void dump_clusters_csv(const std::vector<glm::dvec2>& points,
                       const std::vector<std::vector<u32>>& clusters,
                       std::string_view filename) 
{
  auto file = std::ofstream(filename.data());
  auto point_cluster = std::vector<i32>(points.size(), -1);
  for (auto i = 0ul; i < clusters.size(); ++i) 
  {
    for (auto idx : clusters[i]) 
    {
      if (idx < points.size())
        point_cluster[idx] = static_cast<i32>(i);
    }
  }

  for (auto i = 0ul; i < points.size(); ++i)
    file << points[i].x << "," << points[i].y << "," << point_cluster[i] << "\n";
}