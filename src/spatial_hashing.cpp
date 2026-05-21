#include "spatial_hashing.hpp"

#include <glm/geometric.hpp>

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