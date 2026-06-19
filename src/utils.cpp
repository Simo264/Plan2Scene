#include "utils.hpp"



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

void dump_faces_csv(const std::vector<Face>& faces, std::string_view filename) 
{
  auto file = std::ofstream(filename.data());
  file << std::fixed << std::setprecision(6);

  auto face_id = 0;
  for (const auto& face : faces) 
  {
    file << static_cast<int>(face.type) << "," << face_id;
    for (const auto& v : face.vertices) 
      file << "," << v.x << "," << v.y;
    file << "\n";
    face_id++;
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
