#include "mesh_builder.hpp"

void add_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, IndexedMesh& mesh) 
{
  auto base = mesh.vertices.size();

  mesh.vertices.push_back(v0);
  mesh.vertices.push_back(v1);
  mesh.vertices.push_back(v2);
  
  mesh.indices.push_back(base + 0);
  mesh.indices.push_back(base + 1);
  mesh.indices.push_back(base + 2);
}

void create_floor(const std::vector<p2t::Triangle*>& triangles, IndexedMesh& mesh)
{
  for (const auto& tri : triangles)
  {
    auto p0 = tri->GetPoint(0);
    auto p1 = tri->GetPoint(1);
    auto p2 = tri->GetPoint(2);
    
    auto v0 = Vertex{ .position={p0->x, p0->y, 0.f}, .normal={0.f, 0.f, 1.f}};
    auto v1 = Vertex{ .position={p1->x, p1->y, 0.f}, .normal={0.f, 0.f, 1.f}};
    auto v2 = Vertex{ .position {p2->x, p2->y, 0.f}, .normal={0.f, 0.f, 1.f}};

    add_triangle(v0, v1, v2, mesh);
  }
}

void create_ceiling(const std::vector<p2t::Triangle*>& triangles, float H, IndexedMesh& mesh)
{
  for (const auto& tri : triangles)
  {
    auto p0 = tri->GetPoint(0);
    auto p1 = tri->GetPoint(1);
    auto p2 = tri->GetPoint(2);
    
    auto v0 = Vertex{ .position={p0->x, p0->y, H}, .normal={0.f, 0.f, -1.f}};
    auto v1 = Vertex{ .position={p1->x, p1->y, H}, .normal={0.f, 0.f, -1.f}};
    auto v2 = Vertex{ .position {p2->x, p2->y, H}, .normal={0.f, 0.f, -1.f}};

    // reverse winding order to have the normal pointing downwards
    add_triangle(v0, v2, v1, mesh);
  }
}

void extrude_walls(const Polyline& polyline, float H, IndexedMesh& mesh)
{
  const auto& points = polyline.points;
  auto n = points.size();
  for(auto i = 0; i < n; ++i)
  {
    auto p1 = points.at(i);
    auto p2 = points.at((i + 1) % n);

    auto BL = Vertex{ .position={p1.x, p1.y, 0.f} };
    auto BR = Vertex{ .position={p2.x, p2.y, 0.f} };
    auto TR = Vertex{ .position={p2.x, p2.y,  H } };
    auto TL = Vertex{ .position={p1.x, p1.y,  H } };

    // outward normal: edge direction rotated 90 degrees
    auto dx = p2.x - p1.x;
    auto dy = p2.y - p1.y;
    auto len = std::sqrt(dx * dx + dy * dy);
    // That’s correct ONLY if your polygon is CCW
    auto nx = dy / len;   // rotate CCW contour edge 90° clockwise (outward)
    auto ny = -dx / len;

    auto normal = Vec3{ nx, ny, 0.f };
    BL.normal = normal;
    BR.normal = normal;
    TR.normal = normal;
    TL.normal = normal;

    // two triangles forming the quad, CCW winding viewed from outside
    add_triangle(BL, BR, TR, mesh);
    add_triangle(BL, TR, TL, mesh);
  }
}