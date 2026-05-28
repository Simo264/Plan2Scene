#include "spatial_hashing.hpp"

#include <glm/geometric.hpp>
#include <stdexcept>

VertexId SpatialHash::snap(glm::dvec2 p)
{
  auto cell = get_cell(p);

  // Query: search 3x3 neighbourhood for an existing close vertex
  for (auto dx = -1; dx <= 1; dx++) 
  {
    for (auto dy = -1; dy <= 1; dy++) 
    {
      auto it = m_grid.find(CellCoord{ cell.x + dx, cell.y + dy });
      if (it == m_grid.end()) 
        continue;

      // If found, return its id (reuse it)
      for (auto vertex : it->second) 
      {
        if (glm::distance(m_vertices[vertex], p) < m_epsilon)
          return vertex;
      }
    }
  }

  // If no existing vertex found => create a new one
  auto new_vertex = static_cast<VertexId>(m_vertices.size());
  m_vertices.push_back(glm::dvec2{ p });

  auto [it, inserted] = m_grid.try_emplace(cell);
  it->second.push_back(new_vertex);
  return new_vertex;
}

VertexId SpatialHash::find_nearest(glm::dvec2 p) const
{
  if (m_vertices.empty()) 
    throw std::runtime_error("No vertices in hash, cannot find nearest");
  
  VertexId best_idx = 0;
  auto best_dist2 = glm::length(m_vertices[0] - p);
  for (auto i = 1u; i < m_vertices.size(); ++i) 
  {
    auto d2 = glm::length(m_vertices[i] - p);
    if (d2 < best_dist2) 
    {
      best_dist2 = d2;
      best_idx = static_cast<VertexId>(i);
    }
  }
  return best_idx;
}