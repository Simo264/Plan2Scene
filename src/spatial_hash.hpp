#pragma once

#include "types.hpp"
#include "geometry.hpp"

#include <unordered_map>

using VertexId = u32;

struct GraphVertex
{
  glm::dvec2 position;
};

struct GraphEdge
{
  VertexId v1, v2;
  LayerType layer{ LayerType::NONE };
};

struct CellCoord 
{
  i32 x, y;
};

struct CellCoordHash 
{
  size_t operator()(CellCoord c) const 
  {
  }

};

class SpatialHash
{
public:
  explicit SpatialHash(f64 epsilon);
  
  VertexId snap(glm::dvec2 p);

  auto& vertices() const { return m_vertices; }

  void insert(VertexId vertex, glm::dvec2 position);

  std::vector<VertexId> query(glm::dvec2 position) const;
  
  auto get_cell(glm::dvec2 p) { return CellCoord{
      static_cast<i32>(std::floor(p.x / m_epsilon)),
      static_cast<i32>(std::floor(p.y / m_epsilon))
    };
  }

private:
  // Used both for insertion and query
  f64 m_epsilon;

  // We want to use CellCoord as key in unordered_map.
  // Maps each occupied cell to the list of vertex indices that fall inside it.
  std::unordered_map<CellCoord, std::vector<VertexId>, CellCoordHash> m_grid;

  std::vector<GraphVertex> m_vertices;
};