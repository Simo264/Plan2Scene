#include <print>
#include <filesystem>
#include <vector>
#include <cmath>

// #include "render_primitive.hpp"
#include "drw_parser.hpp"
#include "primitives.hpp"

constexpr auto epsilon = 1e-4; // tollerance for merging points

auto distance(Vec2 p1, Vec2 p2) -> float { return std::hypot(p1.x - p2.x, p1.y - p2.y); }

int main(int argc, char** argv) 
{
  if(argc < 2)
  {
    std::println("Usage: {} <input.dxf>", argv[0]);
    return 1;
  }
  
  auto file_path = argv[1];
  if(!std::filesystem::exists(file_path))
  {
    std::println("File not found: {}", file_path);
    return 1;
  }
  
  // --- Step 1: parsing DXF file to retrieve segments and polylines ---
  // -------------------------------------------------------------------
  auto parser = DRWParser{};
  auto dxf = dxfRW(file_path);
  if (!dxf.read(&parser, false))
  {
    std::println("Error reading DXF file (code: {}): {}", static_cast<int>(dxf.getError()), file_path);
    return 1;
  }

  auto& segments = parser.segments;
  auto& polylines = parser.polylines;
  std::println("Successfully parsed DXF file: segments: {}, polylines: {}", segments.size(), polylines.size());

  // We have unordered disconnected segments?
  // The triangulation library needs an ordered sequence of vertices forming a closed polygon.
  // We must convert this unordered segments into ordered closed contour.
  if(!segments.empty())
  {
    for(const auto& seg : segments)
      std::println("Segment: p1 =({}, {}), p2 = ({}, {})", seg.p1.x, seg.p1.y, seg.p2.x, seg.p2.y);
    
    // espilon merging of points to merge segments that are close enough to be considered connected.
    // Two points closer than epsilon become the same point.
    std::println("todo: merging points...");
    // Once points are snapped, we must build an adjacency graph
    std::println("todo: chaining segments...");
  }
  // We have polylines? Then we already have an ordered contour.
  else if(!polylines.empty())
  {
    auto& polyline = polylines.front();
    auto& points = polyline.points;
    
    // Is polyline closed:
    // We should check the distance between them v[0] and v[last] and if their distance is less than 
    // epsilon they represent the same logical point. 
    // We can drop the last vertex so the contour doesn't have a near-duplicate.
    if(polyline.closed)
    {
      auto& first = points.front();
      auto& last = points.back();
      std::println("Polyline is closed. First point: ({}, {}), last point: ({}, {})", first.x, first.y, last.x, last.y);
      if(distance(first, last) < epsilon)
      {
        std::println("First and last point are closer than epsilon. Drop the last point to avoid near-duplicate.");
        points.pop_back();
      }
    }
    // Polyline is open:
    // we should check if the first and last point are close enough to be considered the same point.
    else 
    {
      auto& first = points.front();
      auto& last = points.back();
      std::println("Polyline is not closed. First point: ({}, {}), last point: ({}, {})", first.x, first.y, last.x, last.y);
      if(distance(first, last) < epsilon)
      {
        std::println("First and last point are closer than epsilon. Drop the last point to avoid near-duplicate and consider it as closed.");
        points.pop_back();
        polyline.closed = true;
      }
      else 
      {
        std::println("First and last point are not closer than epsilon. Exit with error because we need a closed contour for triangulation.");
        return 1;
      }
    }
  }
  
  
  
  
  return 0;
  
  //PrimitivesExample app{{ argc, argv }};
  //return app.exec();
}