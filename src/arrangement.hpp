#pragma once

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_observer.h>

#include "geometry.hpp"
#include "spatial_hashing.hpp"

using Kernel    = CGAL::Exact_predicates_exact_constructions_kernel;
using Traits    = CGAL::Arr_segment_traits_2<Kernel>;
using Point_2   = Traits::Point_2;
using Segment_2 = Traits::X_monotone_curve_2;

// --- Custom DCEL items ---
class VertexBasePoint2 : public CGAL::Arr_vertex_base<Point_2> {};

class CustomHalfEdge : public CGAL::Arr_halfedge_base<Segment_2> {
public:
  LayerType layer{ LayerType::NONE };
};

class CustomFace : public CGAL::Arr_face_base {};
struct CustomDCEL : public CGAL::Arr_dcel_base<VertexBasePoint2, CustomHalfEdge, CustomFace> {};
using Arrangement = CGAL::Arrangement_2<Traits, CustomDCEL>;


// --- Output structures ---
struct FaceEdge 
{
  glm::dvec2 p1, p2;
  LayerType layer;
};

struct PlanarFace 
{
  std::vector<FaceEdge> boundary; // ordered CCW
};

class LayerObserver : public CGAL::Arr_observer<Arrangement>
{
public:
  LayerObserver() : m_current_layer{ LayerType::NONE } {}
  
   void after_create_edge(Halfedge_handle e) override;
   void after_split_edge(Halfedge_handle e1, Halfedge_handle e2) override;
   
private:
  LayerType m_current_layer; 
};


std::vector<PlanarFace> build_arrangement(const std::vector<GraphVertex>& vertices,
                                          const std::vector<GraphEdge>& edges);