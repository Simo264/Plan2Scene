#include "arrangement.hpp"

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
  // CGAL exact predicates — no epsilon needed
  if (CGAL::collinear(A, B, M) == false) return false;
  return CGAL::collinear_are_ordered_along_line(A, M, B);
}


auto build_arrangement(const std::vector<GraphVertex>& vertices, 
                       const std::vector<GraphEdge>& edges) -> Arrangement
{
    // ----------------------------------------------------------------
    // 1. Convert GraphEdges to CGAL Segment2, skip degenerate ones
    // ----------------------------------------------------------------
    struct TaggedSegment { Segment2 segment; LayerType layer; };

    auto tagged = std::vector<TaggedSegment>{};
    tagged.reserve(edges.size());

    for (const auto& e : edges)
    {
        const auto& p1 = vertices[e.v1].position;
        const auto& p2 = vertices[e.v2].position;

        Point2 A = glm_to_cgal(p1);
        Point2 B = glm_to_cgal(p2);

        // Skip degenerate segments (should not exist after snapping,
        // but guard anyway)
        if (A == B) continue;

        tagged.push_back(TaggedSegment{
            .segment = Segment2(A, B),
            .layer   = e.layer
        });
    }

    // ----------------------------------------------------------------
    // 2. Insert all segments — CGAL resolves all T-junctions and
    //    intersections internally
    // ----------------------------------------------------------------
    Arrangement arr;

    auto raw_segments = std::vector<Segment2>{};
    raw_segments.reserve(tagged.size());
    for (const auto& ts : tagged)
        raw_segments.push_back(ts.segment);

    CGAL::insert(arr, raw_segments.begin(), raw_segments.end());

    // ----------------------------------------------------------------
    // 3. Propagate LayerType onto each halfedge
    //
    //    For every halfedge h in the arrangement, its underlying curve
    //    is a sub-segment of exactly one of our original tagged segments.
    //    We find that parent segment and assign its LayerType.
    //
    //    We set the same value on both h and h->twin() because the
    //    layer is a property of the wall/door/window, not of direction.
    // ----------------------------------------------------------------
    for (auto eit = arr.edges_begin(); eit != arr.edges_end(); ++eit)
    {
        // The curve stored on the halfedge after insertion is a
        // sub-segment (or the full segment) of one of our inputs.
        // Its source and target are exact CGAL points.
        const Point2& src = eit->source()->point();
        const Point2& tgt = eit->target()->point();

        // Midpoint of this halfedge — used to identify the parent
        // segment unambiguously even when src or tgt sit on a
        // junction shared by multiple segments.
        // EPECK supports exact arithmetic on midpoints via:
        Point2 mid = CGAL::midpoint(src, tgt);

        LayerType layer = LayerType::NONE;

        for (const auto& ts : tagged)
        {
            const Point2& A = ts.segment.source();
            const Point2& B = ts.segment.target();

            if (point_on_segment(A, B, mid))
            {
                layer = ts.layer;
                break;
            }
        }

        // Set on both twins — layer belongs to the edge, not the direction
        eit->set_data(layer);
        eit->twin()->set_data(layer);
    }

    return arr;
}


auto extract_faces(const Arrangement& arr) -> std::vector<Face>
{
    auto faces = std::vector<Face>{};

    for (auto fit = arr.faces_begin(); fit != arr.faces_end(); ++fit)
    {
        // Skip the unbounded face
        if (fit->is_unbounded()) continue;

        // Skip faces with no outer boundary (should not happen
        // in a well-formed arrangement, but guard anyway)
        if (!fit->has_outer_ccb()) continue;

        auto face = Face{};

        // Walk the outer CCB (counter-clockwise boundary chain)
        // Each step gives us one halfedge of the face boundary
        auto curr = fit->outer_ccb();
        auto first = curr;

        do {
            face.vertices.push_back(CGAL_to_glm(curr->source()->point()));
            face.edge_layers.push_back(curr->data());
            ++curr;
        } while (curr != first);

        // Discard degenerate faces
        if (face.vertices.size() < 3)
            continue;

        faces.push_back(std::move(face));
    }

    return faces;
}