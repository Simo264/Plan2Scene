#pragma once

#include "types.hpp"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arr_extended_dcel.h>

#include <algorithm>

using Kernel   = CGAL::Exact_predicates_exact_constructions_kernel;
using Traits   = CGAL::Arr_segment_traits_2<Kernel>;
using Point2   = Traits::Point_2;
using Segment2 = Traits::X_monotone_curve_2;

using Dcel        = CGAL::Arr_extended_dcel<Traits, int, LayerType, int>;
using Arrangement = CGAL::Arrangement_2<Traits, Dcel>;

struct Face
{
  std::vector<glm::dvec2> vertices;
  std::vector<LayerType> edge_layers;
};

auto build_arrangement(
  const std::vector<GraphVertex>& vertices,
  const std::vector<GraphEdge>& edges) -> Arrangement;

auto extract_faces(const Arrangement& arr) -> std::vector<Face>; 

auto filter_faces_by_area(const std::vector<Face>& faces, f32 threshold = 1.0) -> std::vector<u32>;