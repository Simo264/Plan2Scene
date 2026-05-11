#include <algorithm>
#include <memory>
#include <print>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "geometry.hpp"
#include "glm/trigonometric.hpp"
#include "io/drw_parser.hpp"
#include "io/gltf_exporter.hpp"
#include "graphics/mesh_visualizer.hpp"

#include <poly2tri/sweep/cdt.h>
#include <glm/geometric.hpp>

constexpr auto epsilon = static_cast<f64>(1e-4);

void build_floor(std::vector<Vertex>& out_vertices, 
                 std::vector<u32>& out_indices,
                 const std::vector<p2t::Triangle*> triangle_list)
{
  for (const auto& tri : triangle_list)
  {
    for (auto i = 0; i < 3; ++i) 
    {
      // poly2tri gives CCW winding in XY, so we emit points in order 0,1,2 which stays CCW
      auto p = tri->GetPoint(i);
      auto idx = static_cast<u32>(out_vertices.size());
      out_indices.push_back(idx);

      auto v = Vertex{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = 0.f;
      v.position.z = static_cast<f32>(p->y);
      v.normal = {0.f, 1.f, 0.f},
      out_vertices.push_back(v);
    }
  }
}

void build_ceil(std::vector<Vertex>& out_vertices, 
                std::vector<u32>& out_indices,
                f32 H,
                const std::vector<p2t::Triangle*> triangle_list)
{
  // The ceiling is almost identical to the floor with y = H and normal {0, -1, 0}.
  // However there's one subtle but important issue: the winding order must be reversed.
  for (const auto& tri : triangle_list)
  {
    for (auto i = 2; i >= 0; --i) 
    {
      auto p = tri->GetPoint(i);
      auto idx = static_cast<u32>(out_vertices.size());
      out_indices.push_back(idx);

      auto v = Vertex{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = H;
      v.position.z = static_cast<f32>(p->y);
      v.normal = {0.f, -1.f, 0.f},  // down, into the room
      out_vertices.push_back(v);
    }
  }
}

auto compute_outer_polygon(const std::vector<glm::dvec2>& points, f32 thickness) 
{
    if (points.size() < 3 || thickness == 0.f)
        return points;  // fallback

    std::vector<glm::dvec2> outer;
    const size_t n = points.size();

    // Precompute outward normals for each edge
    std::vector<glm::dvec2> out_normals(n);
    for (size_t i = 0; i < n; ++i) {
        const auto& p1 = points[i];
        const auto& p2 = points[(i + 1) % n];
        glm::dvec2 edge = p2 - p1;               // 2D edge
        // Outward normal: rotate (edge) by -90° (because interior is left of edge for CCW)
        // For CCW polygon, cross(edge, up) gave inward normal. So outward = (-N) i.e. rotate clockwise.
        // In 2D: outward = glm::dvec2(edge.y, -edge.x) normalized.
        glm::dvec2 out = glm::normalize(glm::dvec2(edge.y, -edge.x));
        // Verify orientation: for CCW polygon, edge.y inward-normal, -edge.y outward. But let's use cross logic for consistency.
        // Better: use 3D cross as before: inward = cross( (edge.x,0,edge.y), (0,1,0) ) = (edge.y, 0, -edge.x) => 2D inward = (edge.y, -edge.x). So outward = (-edge.y, edge.x).
        // Let's compute outward as rotate(edge, +90°):
        out = glm::dvec2(-edge.y, edge.x);   // rotate 90° CCW? Wait: In 2D, rotating (x,y) by +90° gives (-y, x). For CCW edge, interior is left => left normal = (-y, x) (that's cross(edge,z)?). Let's test: edge=(1,0) -> left normal = (0,1) which points left of direction. In XZ plane with Y up, inward normal was cross(edge,up) = cross((dx,0,dz), (0,1,0)) = (dz,0,-dx). In 2D this is (dz, -dx). So inward = (dz, -dx). Outward = (-dz, dx). That's exactly rotate by -90°: (dx, dz) rotated -90° gives (dz, -dx)? No: rotate (x,y) by -90° gives (y, -x). So inward = (dz, -dx) = (-(-dx), -dx?) let's just compute consistently.
        // Let's use the 3D cross analog: outward = -inward = (-dz, dx). That is ( -edge.y, edge.x )? edge=(dx, dz) -> (-dz, dx). Yes, outward = glm::dvec2(-edge.y, edge.x). So that's our outward normal.
        out_normals[i] = glm::normalize(glm::dvec2(-edge.y, edge.x));
    }

    for (size_t i = 0; i < n; ++i) {
        // Get this edge outward normal and the previous edge outward normal
        const auto& n1 = out_normals[(i + n - 1) % n];  // previous edge outward
        const auto& n2 = out_normals[i];                // current edge outward

        // Intersection of two offset lines:
        // Line 1 (previous edge): passes through points[i] + thickness * n1, direction = (points[i] - points[i-1])
        // Line 2 (current edge): passes through points[i] + thickness * n2, direction = (points[i+1] - points[i])
        // The intersection is the new outer vertex.
        // We'll use a robust 2D line intersection.
        const auto& p = points[i];
        const auto& p_prev = points[(i + n - 1) % n];
        const auto& p_next = points[(i + 1) % n];

        glm::dvec2 a1 = p_prev + f64(thickness) * n1;
        glm::dvec2 a2 = p + f64(thickness) * n1;  // point on line 1 (but actually line1 passes through a1 along direction (p - p_prev) )
        glm::dvec2 b1 = p + f64(thickness) * n2;
        glm::dvec2 b2 = p_next + f64(thickness) * n2;  // line2 passes through b1 along direction (p_next - p)

        // Solve a1 + t * (a2 - a1) = b1 + s * (b2 - b1) for t.
        glm::dvec2 dir1 = p - p_prev;   // direction of edge1 (from p_prev to p)
        glm::dvec2 dir2 = p_next - p;   // direction of edge2 (from p to p_next)

        // Actually line1: point = p + thickness * n1, direction = (p - p_prev)  (because we shifted the whole edge)
        // Let's define L1(t) = (p + thickness*n1) + t * (p - p_prev)
        // L2(s) = (p + thickness*n2) + s * (p_next - p)
        // Intersection when L1(t) = L2(s)
        glm::dvec2 A = p + f64(thickness) * n1;
        glm::dvec2 B = p + f64(thickness) * n2;
        glm::dvec2 v = p - p_prev;   // direction of line1
        glm::dvec2 w = p_next - p;   // direction of line2

        // Solve linear system: A + t*v = B + s*w  => t*v - s*w = B - A
        double det = v.x * (-w.y) - (-w.x) * v.y; // determinant of [v, -w]
        if (fabs(det) < 1e-8) {
            // Degenerate: fallback to average offset
            outer.push_back(p + f64(thickness) * (n1 + n2) / 2.0);
            continue;
        }
        glm::dvec2 rhs = B - A;
        double t = (rhs.x * (-w.y) - (-w.x) * rhs.y) / det;
        // Intersection point:
        glm::dvec2 outer_vertex = A + t * v;
        outer.push_back(outer_vertex);
    }
    return outer;
}

void extrude_walls(std::vector<Vertex>& vertices, 
                   std::vector<u32>& out_indices,
                   f32 H, f32 thickness,
                   const std::vector<glm::dvec2>& wall_points) 
{
    const auto& inner = wall_points;

    // 1. Inner walls with normals point inward
    for (size_t i = 0; i < inner.size(); ++i) 
    {
      auto p1 = inner[i];
      auto p2 = inner[(i + 1) % inner.size()];
      auto edge = glm::vec3(f32(p2.x - p1.x), 0.0f, f32(p2.y - p1.y));
      constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);
      glm::vec3 normal = glm::normalize(glm::cross(edge, up)); // points inward

      Vertex BL{ {f32(p1.x), 0.001f, f32(p1.y)}, normal };
      Vertex BR{ {f32(p2.x), 0.001f, f32(p2.y)}, normal };
      Vertex TR{ {f32(p2.x), H, f32(p2.y)}, normal };
      Vertex TL{ {f32(p1.x), H, f32(p1.y)}, normal };

      u32 base = u32(vertices.size());
      vertices.push_back(BL);
      vertices.push_back(BR);
      vertices.push_back(TR);
      vertices.push_back(TL);

      out_indices.push_back(base + 0);
      out_indices.push_back(base + 1);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 3);
    }

    // 2. Compute outer polygon
    std::vector<glm::dvec2> outer = compute_outer_polygon(inner, thickness);

    // 3. Outer walls (normals outward)
    for (size_t i = 0; i < outer.size(); ++i) 
    {
      auto p1 = outer[i];
      auto p2 = outer[(i + 1) % outer.size()];
      glm::vec3 edge(f32(p2.x - p1.x), 0.0f, f32(p2.y - p1.y));
      // outward normal = -inward (i.e. -cross(edge, up))
      glm::vec3 normal = -glm::normalize(glm::cross(edge, glm::vec3(0.f, 1.0f, 0.f)));

      Vertex BL{ {f32(p1.x), 0.001f, f32(p1.y)}, normal };
      Vertex BR{ {f32(p2.x), 0.001f, f32(p2.y)}, normal };
      Vertex TR{ {f32(p2.x), H, f32(p2.y)}, normal };
      Vertex TL{ {f32(p1.x), H, f32(p1.y)}, normal };

      u32 base = u32(vertices.size());
      vertices.push_back(BL);
      vertices.push_back(BR);
      vertices.push_back(TR);
      vertices.push_back(TL);

      out_indices.push_back(base + 0);
      out_indices.push_back(base + 1);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 3);
    }

    // 4. Bottom caps (y=0) – normals down
    for (size_t i = 0; i < inner.size(); ++i) 
    {
      auto i1 = inner[i];
      auto i2 = inner[(i + 1) % inner.size()];
      auto o1 = outer[i];
      auto o2 = outer[(i + 1) % outer.size()];

      Vertex v0{ {f32(i1.x), 0.001f, f32(i1.y)}, {0.f, -1.f, 0.f} };
      Vertex v1{ {f32(i2.x), 0.001f, f32(i2.y)}, {0.f, -1.f, 0.f} };
      Vertex v2{ {f32(o2.x), 0.001f, f32(o2.y)}, {0.f, -1.f, 0.f} };
      Vertex v3{ {f32(o1.x), 0.001f, f32(o1.y)}, {0.f, -1.f, 0.f} };

      u32 base = u32(vertices.size());
      vertices.push_back(v0);
      vertices.push_back(v1);
      vertices.push_back(v2);
      vertices.push_back(v3);

      out_indices.push_back(base + 0);
      out_indices.push_back(base + 1);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 3);
    }

    // 5. Top caps (y=H) – normals up
    for (size_t i = 0; i < inner.size(); ++i) 
    {
      auto i1 = inner[i];
      auto i2 = inner[(i + 1) % inner.size()];
      auto o1 = outer[i];
      auto o2 = outer[(i + 1) % outer.size()];

      Vertex v0{ {f32(i1.x), H, f32(i1.y)}, {0.f, 1.f, 0.f} };
      Vertex v1{ {f32(o1.x), H, f32(o1.y)}, {0.f, 1.f, 0.f} };  // order swapped for outward face
      Vertex v2{ {f32(o2.x), H, f32(o2.y)}, {0.f, 1.f, 0.f} };
      Vertex v3{ {f32(i2.x), H, f32(i2.y)}, {0.f, 1.f, 0.f} };

      u32 base = u32(vertices.size());
      vertices.push_back(v0);
      vertices.push_back(v1);
      vertices.push_back(v2);
      vertices.push_back(v3);

      out_indices.push_back(base + 0);
      out_indices.push_back(base + 1);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 3);
    }
}

void parse_cad(const std::filesystem::path& filename, 
               std::vector<Vertex>& out_vertices, 
               std::vector<u32>& out_indices)
{
  // --- Step 1: parsing DXF file to retrieve segments and polylines ---
  // -------------------------------------------------------------------
  auto parser = DRWParser{};
  auto dxf = dxfRW(filename.string().c_str());
  if (!dxf.read(&parser, false))
    throw std::runtime_error(std::format("Error reading DXF file (code: {}): {}", static_cast<i32>(dxf.getError()), filename.string()));

  auto& segments = parser.segments;
  auto& polylines = parser.polylines;
  std::println("Successfully parsed DXF file: segments: {}, polylines: {}", segments.size(), polylines.size());
  
  // // We have unordered disconnected segments? 
  // // The triangulation library needs an ordered sequence of vertices forming a closed polygon.
  // // We must convert this unordered segments into ordered closed contour.
  // if(!segments.empty())
  // {
  //   // Two points closer than epsilon become the same point.
  //   std::println("todo: merging points...");
  //   // Once points are snapped, we must build an adjacency graph
  //   std::println("todo: chaining segments...");
  //   throw std::runtime_error("Chaining segments into a closed contour is not implemented yet.");
  // }

  if(polylines.empty())
    throw std::runtime_error("No wall polyline found");

  // With polylines we already have an ordered contour.
  auto& wall_polyline = polylines.front();
  std::println("Wall polyline has {} points.", wall_polyline.points.size());  
  if (wall_polyline.points.size() < 3)
    throw std::runtime_error("Not enough points to triangulate");

  // Is polyline closed: we should check the distance between them v[0] and v[last] and if their 
  // distance is less than epsilon they represent the same logical point. 
  // We can drop the last vertex so the contour doesn't have a near-duplicate.
  if(wall_polyline.closed)
  {
    auto first_point = wall_polyline.points.front();
    auto last_point = wall_polyline.points.back();
    std::println("Polyline is closed. First point: ({}, {}), last point: ({}, {})", first_point.x, first_point.y, last_point.x, last_point.y);
    if(glm::distance(first_point, last_point) < epsilon)
    {
      std::println("Merge the first and last points");
      wall_polyline.points.pop_back();
    }
  }
  else 
  {
    // Polyline is open: we should check if the first and last point are close enough to be considered the same point.
    auto first_point = wall_polyline.points.front();
    auto last_point = wall_polyline.points.back();
    std::println("Polyline is not closed. First point: ({}, {}), last point: ({}, {})", first_point.x, first_point.y, last_point.x, last_point.y);
    if(glm::distance(first_point, last_point) < epsilon)
    {
      std::println("First and last point are closer than epsilon. Drop the last point to avoid near-duplicate and consider it as closed.");
      wall_polyline.points.pop_back();
      wall_polyline.closed = true;
    }
    else 
      throw std::runtime_error("First and last point are not closer than epsilon. Exit with error because we need a closed contour for triangulation.");
  }

  if(parser.unit_scale == 0.0f)
  {
    std::println("Unit scale not specified in DXF header. Detecting unit scale from geometry...");
    parser.unit_scale = detect_unit_scale(wall_polyline.points);
    std::println("Detected unit scale: {}", parser.unit_scale);
  }

  for (auto& p : wall_polyline.points)
  {
    p.x *= parser.unit_scale;
    p.y *= parser.unit_scale;
  } 
  
  // --- Step 2: triangulation of the contour using poly2tri ---
  // ----------------------------------------------------------
  // poly2tri expects the outer polygon to be counter-clockwise (CCW) and holes to be clockwise (CW).
  // If signed_area < 0 the order is CW: we must reverse the vertices before passing to poly2tri.
  auto area = calculate_signed_area(wall_polyline);
  std::println("Signed area of the contour: {}", area);
  if(area < 0.0f)
    std::ranges::reverse(wall_polyline.points);
   
  auto contour = std::vector<p2t::Point*>{};
  contour.reserve(wall_polyline.points.size());
  for(const auto& p : wall_polyline.points)
    contour.push_back(new p2t::Point{p.x, p.y});
    
  auto cdt = p2t::CDT(contour);
  cdt.Triangulate();
  auto triangle_list = cdt.GetTriangles();
  std::println("Triangulation completed. Number of triangles: {}", triangle_list.size());

  // --- Step 3: extrusion and mesh creation ---
  // -------------------------------------------

  // Define the vertices for our mesh
  auto nr_vertices_floor = triangle_list.size() * 3;
  auto nr_vertices_wall = wall_polyline.points.size() * 6; // 2 triangles * 3 vertices per edge
  auto nr_vertices_ceil = nr_vertices_floor; // same as floor 
  out_vertices.reserve(nr_vertices_floor + nr_vertices_wall + nr_vertices_ceil);

  build_floor(out_vertices, out_indices, triangle_list);
  
  constexpr auto H = 3.f;
  constexpr auto thickness = 0.125f;
  extrude_walls(out_vertices, out_indices, H, thickness, wall_polyline.points);
  
  //build_ceil(out_vertices, out_indices, H + 0.001f, triangle_list);
}

int main(int argc, char* argv[])
{
  if(argc != 3)
    throw std::runtime_error(
      "Usage:\n1. /build/Plan2Scene --load <model/input.gltf>\n2. ./build/Plan2Scene --parse <cad/input.dxf>");
  
  auto mode = std::string(argv[1]);
  auto is_parse = mode == "--parse";
  auto is_load = mode == "--load";
  if(!is_load && ! is_parse)
    throw std::runtime_error(
      "Usage example:\n 1. /build/Plan2Scene --load <model/input.gltf>\n2. ./build/Plan2Scene --parse <cad/input.dxf>");

  auto file_path = std::filesystem::path(argv[2]);
  if(!std::filesystem::exists(file_path))
    throw std::runtime_error(std::format("Input file not found: {}", file_path.string()));
  
  auto vertices = std::vector<Vertex>{};
  auto indices = std::vector<u32>{};
  if(is_load)
  {
    import_gltf(file_path, vertices, indices);
  }
  else if(is_parse)
  {
    parse_cad(file_path, vertices, indices);
   
    // exporting mesh in GLTF
    auto gltf_path = file_path.filename().replace_extension("gltf");
    std::println("Model will be exported to: {}", gltf_path.string());
    export_to_gltf(vertices, indices, gltf_path);
  }


  // --- visualize mesh ---
  // ----------------------

  // Get the bounds of the extruded 3D room
  auto bbox = calculate_bounding_box(vertices);
  auto center = (bbox.min + bbox.max) * 0.5f; 
  // Center the model at the origin (0, 0, 0)
  auto transform = Transformation{};
  transform.position = -center;
  transform.update_tranformation();
  
  auto visualizer = MeshVisualizer(1024, 768);
  visualizer.set_mesh(std::make_shared<StaticMesh>(
    vertices.data(), 
    vertices.size(),
    indices.data(),  
    indices.size()
  ));
  visualizer.set_mesh_transform(transform);
  visualizer.camera().eye = { 0.f, 2.f, 10.f };
  visualizer.camera().set_orientation(glm::radians(glm::vec3{ -5.f, 0.f, 0.f }));
  visualizer.render();
  return 0;
}