#include "arrangement.hpp"

void LayerObserver::after_create_edge(Halfedge_handle e) 
{
  e->layer = m_current_layer;
  e->twin()->layer = m_current_layer;
}

void LayerObserver::after_split_edge(Halfedge_handle e1, Halfedge_handle e2) 
{
    // e1 and e2 are the two halves after split their twins already carry the layer from before the split
    auto layer_tag = e1->twin()->layer;
    e1->layer = layer_tag;
    e1->twin()->layer = layer_tag;
    e2->layer = layer_tag;
    e2->twin()->layer = layer_tag;
}


std::vector<PlanarFace> build_arrangement(const std::vector<GraphVertex>& vertices,
                                          const std::vector<GraphEdge>& edges)
{
  
}