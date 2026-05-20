#pragma once

#include "types.hpp"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arr_extended_dcel.h>

using Kernel   = CGAL::Exact_predicates_exact_constructions_kernel;
using Traits   = CGAL::Arr_segment_traits_2<Kernel>;
using Point2   = Traits::Point_2;
using Segment2 = Traits::X_monotone_curve_2;

// Arr_extended_dcel<Traits, VData, HData, FData>
// VData = data attached to each vertex   → we don't need anything, use int as placeholder
// HData = data attached to each halfedge → LayerType, this is what we care about
// FData = data attached to each face     → we don't need anything, use int as placeholder
using Dcel        = CGAL::Arr_extended_dcel<Traits, int, LayerType, int>;
using Arrangement = CGAL::Arrangement_2<Traits, Dcel>;

auto build_arrangement(
  const std::vector<GraphVertex>& vertices,
  const std::vector<GraphEdge>& edges) -> Arrangement;

auto extract_faces(const Arrangement& arr) -> std::vector<Face>; 