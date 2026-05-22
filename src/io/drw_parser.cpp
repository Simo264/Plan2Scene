#include "drw_parser.hpp"

#include <glm/ext/vector_double2.hpp>

#include <print>
#include <algorithm>
#include <string_view>

static auto classify_layer(std::string_view name) 
{
  // if (name == "WALL") 
  //   return LayerType::WALL;
  //  if (name == "WINDOW") 
  //    return LayerType::WINDOW;
  if (name == "DOOR")
    return LayerType::DOOR;
   
  return LayerType::NONE;
} 

void DRWParser::addHeader(const DRW_Header* data)
{
  unit_scale = 0.0f;
  
  if(!data) 
  { 
    std::println("Warning: no header data.");
    return;
  }
  
  auto it = data->vars.find("$INSUNITS"); 
  if(it == data->vars.end())
  {
    std::println("Warning: $INSUNITS not found.");
    return;
  }

  auto units = 0;
  auto variant = it->second;
  if (variant->type() == DRW_Variant::INTEGER)
  {
    units = variant->content.i;
  }
  else
  {
    std::println("Warning: $INSUNITS is not INTEGER.");
    return;
  }
  
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
      std::println("Warning: unknown units.");
      break;
  }
  std::println("Header: INSUNITS = {}, scale = {}", units, unit_scale);
}

void DRWParser::addLayer([[maybe_unused]] const DRW_Layer& data)
{
  // std::println("Layer: layer_name=`{}`", data.name);
}

void DRWParser::addLine(const DRW_Line& data)
{
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); });

  auto layer_type = classify_layer(layer_name);
  if(layer_type == LayerType::WALL ||
     layer_type == LayerType::WINDOW || 
     layer_type == LayerType::DOOR)
  {  
    std::println("Line: layer_name=`{}`", layer_name);

    input_segments.push_back(Segment{
      .p1 = glm::dvec2{ data.basePoint.x, data.basePoint.y },
      .p2 = glm::dvec2{ data.secPoint.x, data.secPoint.y },
      .layer = layer_type
    });
  }
}

void DRWParser::addPolyline(const DRW_Polyline& data)
{
  auto layer_name = data.layer;
  auto is_closed = data.flags & 1;
  auto& vertices = data.vertlist;
  if (vertices.size() < 2)
    return;

  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  auto layer_type = classify_layer(layer_name);
  if(layer_type == LayerType::WALL || 
     layer_type == LayerType::WINDOW || 
     layer_type == LayerType::DOOR)
  {
    std::println("Polyline: layer_name=`{}`, nr_vertices = {}, closed = {}", layer_name, vertices.size(), is_closed);
    
    // Decompose into individual segments: one per consecutive pair of vertices.
    for (auto i = 0u; i < vertices.size() - 1; ++i)
    {
      auto p0 = vertices.at(i);
      auto p1 = vertices.at(i+1);
      input_segments.push_back(Segment{
        .p1 = glm::dvec2{ p0->basePoint.x, p0->basePoint.y },
        .p2 = glm::dvec2{ p1->basePoint.x, p1->basePoint.y },
        .layer = layer_type
      });
    }
  
    // If closed, add the closing segment from last vertex back to first.
    if (is_closed)
    {
      auto p0 = vertices.front();
      auto p_last = vertices.back();
      input_segments.push_back(Segment{
        .p1 = glm::dvec2{ p_last->basePoint.x, p_last->basePoint.y },
        .p2 = glm::dvec2{ p0->basePoint.x, p0->basePoint.y },
        .layer = layer_type
      });
    }   
  }
}

void DRWParser::addLWPolyline(const DRW_LWPolyline& data)
{
  auto layer_name = data.layer;
  auto is_closed = data.flags & 1;
  auto& vertices = data.vertlist;
  if (vertices.size() < 2)
    return;

  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 

  auto layer_type = classify_layer(layer_name);
  if(layer_type == LayerType::WALL || 
     layer_type == LayerType::WINDOW || 
     layer_type == LayerType::DOOR)
  {
    std::println("LWPolyline: layer_name=`{}`, nr_vertices = {}, closed = {}", layer_name, vertices.size(), is_closed);

    // Decompose into individual segments: one per consecutive pair of vertices.
    for (auto i = 0u; i < vertices.size() - 1; ++i)
    {
      auto p0 = vertices.at(i);
      auto p1 = vertices.at(i+1);
      input_segments.push_back(Segment{
        .p1 = glm::dvec2{ p0->x, p0->y },
        .p2 = glm::dvec2{ p1->x, p1->y },
        .layer = layer_type
      });
    }
  
    // If closed, add the closing segment from last vertex back to first.
    if (is_closed)
    {
      auto p0 = vertices.front();
      auto p_last = vertices.back();
      input_segments.push_back(Segment{
        .p1 = glm::dvec2{ p_last->x, p_last->y },
        .p2 = glm::dvec2{ p0->x, p0->y },
        .layer = layer_type
      });
    }   
  }
}

void DRWParser::addInsert(const DRW_Insert& data)
{
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  auto layer_type = classify_layer(layer_name);
  if(layer_type == LayerType::WALL || 
     layer_type == LayerType::WINDOW || 
     layer_type == LayerType::DOOR)
  {
    std::println("Insert: layer_name=`{}`", data.layer);
  }
}

void DRWParser::addArc(const DRW_Arc& data)
{  
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  auto layer_type = classify_layer(layer_name);
  if(layer_type == LayerType::WALL || 
     layer_type == LayerType::WINDOW || 
     layer_type == LayerType::DOOR)
  {
    std::println("Arc: layer_name=`{}`", data.layer);
  }
}

void DRWParser::addPoint(const DRW_Point& data)
{
  auto layer_name = data.layer;
  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  auto layer_type = classify_layer(layer_name);
  if(layer_type == LayerType::WALL || 
     layer_type == LayerType::WINDOW || 
     layer_type == LayerType::DOOR)
  {
    std::println("Point: layer_name=`{}`", data.layer);
  }
}

void DRWParser::addBlock([[maybe_unused]]const DRW_Block& data)
{
  //std::println("Block: layer_name=`{}`", data.layer);
}