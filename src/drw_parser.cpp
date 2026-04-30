#include "drw_parser.hpp"

#include <print>
#include <algorithm>

void DRWParser::addHeader(const DRW_Header* data)
{
  if(!data) 
  { 
    unit_scale = 1.0;
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
    case 1: // inches
      unit_scale = 0.0254;
      break;
    case 4: // millimeters
      unit_scale = 0.001;
      break;
    case 5: // centimeters
      unit_scale = 0.01;
      break;
    case 6: // meters
      unit_scale = 1.0;
      break;
    default:
      unit_scale = 1.0;
      break;
  }
  std::println("Header: INSUNITS = {}, scale = {}", units, unit_scale);
}

void DRWParser::addLine(const DRW_Line& data)
{
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  std::println("Line: layer = {}, p1 =({}, {}), p2 = ({}, {})", layer_name, data.basePoint.x, data.basePoint.y, data.secPoint.x, data.secPoint.y);
  if(layer_name.contains("WALL"))
  {  
    auto seg = Segment{};
    seg.p1 = { static_cast<float>(data.basePoint.x)  * unit_scale, static_cast<float>(data.basePoint.y) * unit_scale };
    seg.p2 = { static_cast<float>(data.secPoint.x) * unit_scale, static_cast<float>(data.secPoint.y) * unit_scale };
    seg.layer = layer_name;
    segments.push_back(seg);
  }
}

void DRWParser::addPolyline(const DRW_Polyline& data)
{
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  std::print("Polyline: layer = {} nr_vertices = {} ", layer_name, data.vertlist.size());
  if(layer_name.contains("WALL"))
  {
    auto poly = Polyline{};
    poly.layer = layer_name;
    poly.closed = data.flags & 1;
    for (const auto& v : data.vertlist)
    {
      auto p = Vec2{};
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
  std::print("LWPolyline: layer = {} nr_vertices = {} ", layer_name, data.vertlist.size());
  if(layer_name.contains("WALL"))
  {
    auto poly = Polyline{};
    poly.layer = layer_name;
    poly.closed = data.flags & 1;
    for (const auto& v : data.vertlist) 
    {
      auto p = Vec2{};
      p.x = v->x * unit_scale;
      p.y = v->y * unit_scale;
      poly.points.push_back(p);
    }
    polylines.push_back(poly);
  }
  std::println();
}