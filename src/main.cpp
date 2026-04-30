#include <print>
#include <filesystem>

// #include "render_primitive.hpp"
#include "drw_parser.hpp"
#include "primitives.hpp"

bool are_equals(Vec2 p1, Vec2 p2, double e)
{
  return std::abs(p1.x - p2.x) < e && std::abs(p1.y - p2.y) < e;
}

void merge_points()
{

}


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
  std::println("Successfully parsed DXF file. Segments: {}, Polylines: {}", segments.size(), polylines.size());

  // --- Step 2: convert polylines to segments ---
  // ---------------------------------------------
  if(!polylines.empty())
  {
    
  }


  // --- Step 3: epsilon merging ---
  // -------------------------------
  constexpr double EPS = 1e-5;
  std::vector<Vec2> unique_points;


  // --- Step 4: build an adjacency graph ---
  // ----------------------------------------


  return 0;
  
  //PrimitivesExample app{{ argc, argv }};
  //return app.exec();
}