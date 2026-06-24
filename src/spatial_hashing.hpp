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
  SpatialHash(f64 epsilon = 1e-4) : m_epsilon{ epsilon} {}

  VertexId snap(glm::dvec2 p);
  VertexId find_nearest(glm::dvec2 p) const;
  
  auto& vertices() { return m_vertices; }

private:  
  auto get_cell(glm::dvec2 p) const -> CellCoord
  {
    return CellCoord {
      static_cast<i32>(std::floor(p.x / m_epsilon)),
      static_cast<i32>(std::floor(p.y / m_epsilon))
    };
  }

  
  f64 m_epsilon;

  // We want to use CellCoord as key in unordered_map.
  // Maps each occupied cell to the list of vertex indices that fall inside it.
  std::unordered_map<CellCoord, std::vector<VertexId>, CellCoordHash> m_grid;

  std::vector<glm::dvec2> m_vertices;
};