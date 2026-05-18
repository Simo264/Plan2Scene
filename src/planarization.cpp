#include "planarization.hpp"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>

using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;

using Point2 = Kernel::Point_2;
using Segment2 = Kernel::Segment_2;

using Traits2 = CGAL::Arr_segment_traits_2<Kernel>;
using Arrangement2 = CGAL::Arrangement_2<Traits2>;