#include <algorithm>
#include <print>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <exception>

// #include "render_primitive.hpp"
#include "drw_parser.hpp"
#include "poly2tri/common/shapes.h"
#include "poly2tri/sweep/cdt.h"
#include "primitives.hpp"

#include "poly2tri/poly2tri.h"

constexpr auto epsilon = 1e-4; // tollerance for merging points

auto distance(Vec2 p1, Vec2 p2) -> float { return std::hypot(p1.x - p2.x, p1.y - p2.y); }

auto signed_area(const Polyline& contour) -> float
{
  const auto& points = contour.points;
  // Compute the signed area of the contour using the shoelace formula.
  auto area = 0.0f;
  for(auto i = 0u; i < points.size(); ++i)
  {
    const auto& p1 = points.at(i);
    const auto& p2 = points.at((i + 1) % points.size());
    area += (p1.x * p2.y - p2.x * p1.y);
  }
  return area * 0.5f;
}

int main(int argc, char** argv) 
{
  if(argc < 2)
    throw std::runtime_error(std::format("No input file provided. Usage: {} <input.dxf>", argv[0]));
  
  auto file_path = argv[1];
  if(!std::filesystem::exists(file_path))
    throw std::runtime_error(std::format("Input file not found: {}", file_path));
  
  // --- Step 1: parsing DXF file to retrieve segments and polylines ---
  // -------------------------------------------------------------------
  auto parser = DRWParser{};
  auto dxf = dxfRW(file_path);
  if (!dxf.read(&parser, false))
    throw std::runtime_error(std::format("Error reading DXF file (code: {}): {}", static_cast<int>(dxf.getError()), file_path));

  auto& segments = parser.segments;
  auto& polylines = parser.polylines;
  std::println("Successfully parsed DXF file: segments: {}, polylines: {}", segments.size(), polylines.size());

  auto& wall_polyline = polylines.front();
  auto& wall_points = wall_polyline.points;
  std::println("Wall polyline has {} points.", wall_points.size());
  
  // We have unordered disconnected segments?
  // The triangulation library needs an ordered sequence of vertices forming a closed polygon.
  // We must convert this unordered segments into ordered closed contour.
  if(!segments.empty())
  {
    //for(const auto& seg : segments)
    //  std::println("Segment: p1 =({}, {}), p2 = ({}, {})", seg.p1.x, seg.p1.y, seg.p2.x, seg.p2.y);
    
    // espilon merging of points to merge segments that are close enough to be considered connected.
    // Two points closer than epsilon become the same point.
    std::println("todo: merging points...");
    // Once points are snapped, we must build an adjacency graph
    std::println("todo: chaining segments...");
    throw std::runtime_error("Chaining segments into a closed contour is not implemented yet.");
  }
  // We have polylines? Then we already have an ordered contour.
  else if(!polylines.empty())
  {
    // Is polyline closed: we should check the distance between them v[0] and v[last] and if their 
    // distance is less than epsilon they represent the same logical point. 
    // We can drop the last vertex so the contour doesn't have a near-duplicate.
    if(wall_polyline.closed)
    {
      auto first_point = wall_points.front();
      auto last_point = wall_points.back();
      std::println("Polyline is closed. First point: ({}, {}), last point: ({}, {})", first_point.x, first_point.y, last_point.x, last_point.y);
      if(distance(first_point, last_point) < epsilon)
      {
        std::println("First and last point are closer than epsilon. Drop the last point to avoid near-duplicate.");
        wall_points.pop_back();
      }
    }
    // Polyline is open: we should check if the first and last point are close enough to be 
    // considered the same point.
    else 
    {
      auto first_point = wall_points.front();
      auto last_point = wall_points.back();
      std::println("Polyline is not closed. First point: ({}, {}), last point: ({}, {})", first_point.x, first_point.y, last_point.x, last_point.y);
      if(distance(first_point, last_point) < epsilon)
      {
        std::println("First and last point are closer than epsilon. Drop the last point to avoid near-duplicate and consider it as closed.");
        wall_points.pop_back();
        wall_polyline.closed = true;
      }
      else 
        throw std::runtime_error("First and last point are not closer than epsilon. Exit with error because we need a closed contour for triangulation.");
    }
  }
  
  // --- Step 2: triangulation of the contour using poly2tri---
  // ----------------------------------------------------------
  // Compute signed area to determine orientation
  // poly2tri expects the outer polygon to be counter-clockwise (CCW) and holes to be clockwise (CW).
  // If signed_area < 0 the order is CW: we must reverse the vertices before passing to poly2tri.
  // Otherwise, if signed_area > 0 the order is CCW and we can pass the vertices as they are.
  if (wall_points.size() < 3)
    throw std::runtime_error("Not enough points to triangulate");

  auto area = signed_area(wall_polyline);
  std::println("Signed area of the contour: {}", area);
  if(area < 0)
    std::ranges::reverse(wall_points);
   
  auto contour = std::vector<p2t::Point*>{};
  contour.reserve(wall_points.size());
  for(const auto& p : wall_points)
    contour.push_back(new p2t::Point{p.x, p.y});
    
  auto cdt = p2t::CDT(contour);
  cdt.Triangulate();
  auto triangle_list = cdt.GetTriangles();
  std::println("Triangulation completed. Number of triangles: {}", triangle_list.size());
  for (const auto& tri : triangle_list) 
  {
    auto p0 = tri->GetPoint(0);
    auto p1 = tri->GetPoint(1);
    auto p2 = tri->GetPoint(2);
  }


  return 0;
  
  //PrimitivesExample app{{ argc, argv }};
  //return app.exec();
}