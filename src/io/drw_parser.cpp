#include "drw_parser.hpp"

#include <glm/ext/vector_double2.hpp>

#include <print>
#include <algorithm>

void DRWParser::addHeader(const DRW_Header* data)
{
  if(!data) 
  { 
    unit_scale = 0.0f;
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
    case 1:
      unit_scale = 0.0254f; // inches
      break;
    case 4: 
      unit_scale = 0.001f; // millimeters
      break;
    case 5:
      unit_scale = 0.01f; // centimeters
      break;
    case 6:
      unit_scale = 1.0f; // meters
      break;
    default:
      unit_scale = 0.0f; // unknown
      std::println("Warning: unknown units. Try to detect the actual unit from the geometry.");
      break;
  }
  std::println("Header: INSUNITS = {}, scale = {}", units, unit_scale);
}

void DRWParser::addLine(const DRW_Line& data)
{
  auto layer_name = data.layer;
  std::println("Line: layer_name=`{}`", layer_name);
  
 //  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
 //  std::println("Line: layer = {}", layer_name);
 //  if(layer_name.contains("WALL"))
 //  {  
 //    auto seg = Segment{};
 //    seg.p1 = { data.basePoint.x, data.basePoint.y };
 //    seg.p2 = { data.secPoint.x, data.secPoint.y };
 //    segments.push_back(seg);
 //  }
}// 

void DRWParser::addPolyline(const DRW_Polyline& data)
{
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  auto poly = Polyline{};
  poly.closed = data.flags & 1;
  std::println("Polyline: layername=`{}`, nr_vertices = {}, closed = {}", layer_name, data.vertlist.size(), poly.closed);
  if(layer_name.contains("WALL"))
  {
    for (const auto& v : data.vertlist)
    {
      auto p = glm::dvec2{v->basePoint.x, v->basePoint.y};
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
  std::println("LWPolyline: layer_name=`{}`, nr_vertices = {}, closed = {}", layer_name, data.vertlist.size(), poly.closed);
  if(layer_name.contains("WALL"))
  {
    for (const auto& v : data.vertlist) 
    {
      auto p = glm::dvec2{v->x, v->y};
      poly.points.push_back(p);
    }
    polylines.push_back(poly);
  }
}