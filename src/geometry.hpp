#pragma once

#include <vector>

using IndexType = unsigned int;

struct Vec2
{  
  double x{ 0.f }, y{ 0.f };
};

struct Vec3
{  
  double x{ 0.f }, y{ 0.f }, z{ 0.f };
};

struct Segment
{
  Vec2 p1, p2;
};

struct Polyline
{
  std::vector<Vec2> points;
  bool closed{ false };
};

struct Triangle2D
{
  Vec2 p1, p2, p3;
};

struct Triangle3D
{
  Vec3 p1, p2, p3;
};

struct Vertex
{
  Vec3 position;
  Vec3 normal;
  Vec2 texcoord;
};

struct IndexedMesh
{
  std::vector<Vertex> vertices;
  std::vector<IndexType> indices;
};

auto distance(Vec2 p1, Vec2 p2) -> double;
auto signed_area(const Polyline& contour) -> double;
