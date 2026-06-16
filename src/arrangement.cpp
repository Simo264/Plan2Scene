#include "arrangement.hpp"
#include "types.hpp"
#include <print>
#include <vector>
#include <iostream>

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

Arrangement build_arrangement(const std::vector<glm::dvec2>& vertices, 
                       const std::vector<Edge>& edges)
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

    tagged.push_back(TaggedSegment { Segment2(A, B), e.layer });
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
      {
        if (static_cast<int>(ts.layer) > static_cast<int>(best_layer)) 
          best_layer = ts.layer;
      }
    }

    eit->set_data(best_layer);
    eit->twin()->set_data(best_layer);
  }

  return arr;
}

std::vector<Face> extract_faces(const Arrangement& arr)
{
  auto faces = std::vector<Face>{};

  for (auto fit = arr.faces_begin(); fit != arr.faces_end(); ++fit)
  {
    if (fit->is_unbounded()) 
      continue; 
    if (!fit->has_outer_ccb()) 
      continue;

    auto face = Face{};

    auto curr = fit->outer_ccb();
    auto first = curr;
    do 
    {
      face.vertices.push_back(CGAL_to_glm(curr->source()->point()));
      face.edge_layers.push_back(curr->data());
      ++curr;
    } while (curr != first);

    if (face.vertices.size() < 3)
      continue;

    face.type = classify_face(face);
    faces.push_back(std::move(face));
  }

  return faces;
}

FaceType classify_face(const Face& face) 
{
  auto wall_count = 0u;
  auto door_count = 0u;
  auto window_count = 0u;
  for (LayerType layer : face.edge_layers) 
  {
    if (layer == LayerType::WALL)         wall_count++;
    else if (layer == LayerType::DOOR)    door_count++;
    else if (layer == LayerType::WINDOW)  window_count++;
  }

  auto total_edges = static_cast<u32>(face.edge_layers.size());

  // Door face
  
  if (total_edges == 4 && wall_count == 2 && door_count == 2) 
    return FaceType::DOOR;

  // Window face
  
  if (total_edges == 4 && wall_count == 2 && window_count == 2)
    return FaceType::WINDOW;

  // Wall face

  if (wall_count == total_edges) 
    return FaceType::WALL;

  // Room face

  return FaceType::ROOM;
}