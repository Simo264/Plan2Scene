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

using Dcel        = CGAL::Arr_extended_dcel<Traits, int, LayerType, int>;
using Arrangement = CGAL::Arrangement_2<Traits, Dcel>;

Arrangement build_arrangement(const std::vector<glm::dvec2>& vertices,
                       const std::vector<Edge>& edges);

std::vector<Face>  extract_faces(const Arrangement& arr); 

FaceType classify_face(const Face& face);