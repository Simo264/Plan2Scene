#include <print>
#include <filesystem>

// #include "render_primitive.hpp"
#include "drw_parser.hpp"

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
  
  auto parser = DRWParser{};
  auto dxf = dxfRW(file_path);
  if (!dxf.read(&parser, false))
  {
    std::println("Error reading DXF file (code: {}): {}", static_cast<int>(dxf.getError()), file_path);
    return 1;
  }
  
  std::println("Successfully parsed DXF file. Segments: {}, Polylines: {}", parser.segments.size(), parser.polylines.size());
  return 0;
  
  //PrimitivesExample app{{ argc, argv }};
  //return app.exec();
}