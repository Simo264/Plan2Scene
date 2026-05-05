#include "drw_parser.hpp"

#include <glm/ext/vector_double2.hpp>

#include <print>
#include <algorithm>

void DRWParser::addHeader(const DRW_Header* data)
{
  if(!data) 
  { 
    unit_scale = 1.0f;
    std::println("Header: no data. Deafult unit: {}", unit_scale);
    return;
  }
  
  auto it = data->vars.find("$INSUNITS"); 
  if(it == data->vars.end())
  {
    std::println("Warning: $INSUNITS not found");
    return;
  }

  auto units = 0;
  auto variant = it->second;
  if (variant->type() == DRW_Variant::INTEGER) 
    units = variant->content.i;
  else
    std::println("Warning: $INSUNITS is not INTEGER");
  
  switch (units) 
  {
    case 0:  // unknown
      unit_scale = 1.0f;  
      std::println("Warning: unknown units. Set scale to 1.0");
      break;
    case 1: // inches
      unit_scale = 0.0254f;
      break;
    case 4: // millimeters
      unit_scale = 0.001f;
      break;
    case 5: // centimeters
      unit_scale = 0.01f;
      break;
    case 6: // meters
      unit_scale = 1.0f;
      break;
    default:
      unit_scale = 1.0f;
      break;
  }
  std::println("Header: INSUNITS = {}, scale = {}", units, unit_scale);
}

void DRWParser::addLine(const DRW_Line& data)
{
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  std::println("Line: layer = {}", layer_name);
  if(layer_name.contains("WALL"))
  {  
    auto seg = Segment{};
    seg.p1 = { data.basePoint.x  * unit_scale, data.basePoint.y * unit_scale };
    seg.p2 = { data.secPoint.x * unit_scale, data.secPoint.y * unit_scale };
    segments.push_back(seg);
  }
}

void DRWParser::addPolyline(const DRW_Polyline& data)
{
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  auto poly = Polyline{};
  poly.closed = data.flags & 1;
  std::println("Polyline: layer = {}, nr_vertices = {}, closed = {}", layer_name, data.vertlist.size(), poly.closed);
  if(layer_name.contains("WALL"))
  {
    for (const auto& v : data.vertlist)
    {
      auto p = glm::dvec2{};
      p.x = v->basePoint.x * unit_scale;
      p.y = v->basePoint.y * unit_scale;
      poly.points.push_back(p);
    }
    polylines.push_back(poly);
  }
}

void DRWParser::addLWPolyline(const DRW_LWPolyline& data)
{
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  auto poly = Polyline{};
  poly.closed = data.flags & 1;
  std::println("LWPolyline: layer = {}, nr_vertices = {}, closed = {}", layer_name, data.vertlist.size(), poly.closed);
  if(layer_name.contains("WALL"))
  {
    //poly.layer = layer_name;
    for (const auto& v : data.vertlist) 
    {
      auto p = glm::dvec2{};
      p.x = v->x * unit_scale;
      p.y = v->y * unit_scale;
      poly.points.push_back(p);
    }
    polylines.push_back(poly);
  }
}