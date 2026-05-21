#include "arrangement.hpp"
#include <algorithm>
#include <print>
#include <vector>

static inline Point2 glm_to_cgal(const glm::dvec2& p)
{
  return Point2(p.x, p.y);
}
static inline glm::dvec2 CGAL_to_glm(const Point2& p)
{
  return glm::dvec2{ CGAL::to_double(p.x()), CGAL::to_double(p.y()) };
}
// Returns true if the CGAL point M lies on segment (A, B), including at the endpoints.
static inline bool point_on_segment(const Point2& A, const Point2& B, const Point2& M)
{
  if (CGAL::collinear(A, B, M) == false) 
    return false;
  
  return CGAL::collinear_are_ordered_along_line(A, M, B);
}


auto build_arrangement(const std::vector<glm::dvec2>& vertices, 
                       const std::vector<GraphEdge>& edges) -> Arrangement
{
  // Convert GraphEdges to CGAL Segment2
  
  struct TaggedSegment { Segment2 segment; LayerType layer; };

  auto tagged = std::vector<TaggedSegment>{};
  tagged.reserve(edges.size());
  for (const auto& e : edges)
  {
    const auto& p1 = vertices[e.v1];
    const auto& p2 = vertices[e.v2];
    Point2 A = glm_to_cgal(p1);
    Point2 B = glm_to_cgal(p2);

    tagged.push_back(TaggedSegment {
      .segment = Segment2(A, B),
      .layer   = e.layer
    });
  }

  // Insert all segments. CGAL resolves all T-junctions and intersections internally
  
  auto arr = Arrangement{};

  auto raw_segments = std::vector<Segment2>{};
  raw_segments.reserve(tagged.size());
  for (const auto& ts : tagged)
    raw_segments.push_back(ts.segment);

  CGAL::insert(arr, raw_segments.begin(), raw_segments.end());

  // Propagate LayerType onto each halfedge.
  
  for (auto eit = arr.edges_begin(); eit != arr.edges_end(); ++eit)
  {
    const Point2& src = eit->source()->point();
    const Point2& tgt = eit->target()->point();

    // Midpoint of this halfedge
    Point2 mid = CGAL::midpoint(src, tgt);

    auto best_layer = LayerType::NONE;
    for (const auto& ts : tagged)
    {
      const Point2& A = ts.segment.source();
      const Point2& B = ts.segment.target();
      // the priority is DOOR > WINDOW > WALL > NONE
      if (point_on_segment(A, B, mid))
        best_layer = std::max(best_layer, ts.layer);
    }

    eit->set_data(best_layer);
    eit->twin()->set_data(best_layer);
  }

  return arr;
}


auto extract_faces(const Arrangement& arr) -> std::vector<Face>
{
  auto faces = std::vector<Face>{};

  for (auto fit = arr.faces_begin(); fit != arr.faces_end(); ++fit)
  {
    // Skip the unbounded face
    if (fit->is_unbounded()) 
      continue;
    // Skip faces with no outer boundary (should not happen in a well-formed arrangement)
    if (!fit->has_outer_ccb()) 
      continue;

    auto face = Face{};

    // Walk the outer CCB (counter-clockwise boundary chain)
    auto curr = fit->outer_ccb();
    auto first = curr;
    do {
      face.vertices.push_back(CGAL_to_glm(curr->source()->point()));
      face.edge_layers.push_back(curr->data());
      ++curr;
    } while (curr != first);

    if (face.vertices.size() < 3)
      continue;

    faces.push_back(std::move(face));
  }

  return faces;
}


auto filter_faces_by_area(const std::vector<Face>& faces, f32 threshold) -> std::vector<u32>
{
  auto result = std::vector<u32>{};
  for (auto i = 0u; i < faces.size(); i++)
  {
    const auto& face = faces.at(i);
    if (face.vertices.size() < 3)
      continue;

    // Compute signed area via shoelace formula
    f64 area = 0.0;
    const auto n = face.vertices.size();
    for (auto k = 0u; k < n; ++i)
    {
      const auto& a = face.vertices[k];
      const auto& b = face.vertices[(k + 1) % n];
      area += a.x * b.y - b.x * a.y;
    }
    area = std::abs(area) * 0.5;

    // Discard faces below area threshold (slivers, zero-area artifacts)
    if (area < threshold)
    {
      std::println("Discarding face with area {} < {}", area, threshold); 
      continue;
    }
    
    result.push_back(i);
  }

  return result;
}
