#pragma once

#include "types.hpp"

#include <unordered_map>
#include <vector>
#include <cmath>

struct CellCoord 
{
  i32 x, y;
  bool operator==(const CellCoord&) const = default;
};

struct CellCoordHash 
{
  size_t operator()(CellCoord c) const 
  {
    auto hx = std::hash<i32>{}(c.x);
    auto hy = std::hash<i32>{}(c.y);
    return hx ^ (hy * 2654435761u);
  }
};

class SpatialHash
{
public:
  SpatialHash(f64 epsilon) : m_epsilon{ epsilon} {}
  
  auto& vertices() { return m_vertices; }

  VertexId snap(glm::dvec2 p);

  auto get_cell(glm::dvec2 p) const 
  {
    return CellCoord {
      static_cast<i32>(std::floor(p.x / m_epsilon)),
      static_cast<i32>(std::floor(p.y / m_epsilon))
    };
  }

private:
  f64 m_epsilon;

  // We want to use CellCoord as key in unordered_map.
  // Maps each occupied cell to the list of vertex indices that fall inside it.
  std::unordered_map<CellCoord, std::vector<VertexId>, CellCoordHash> m_grid;

  std::vector<glm::dvec2> m_vertices;
};