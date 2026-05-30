#include "drw_parser.hpp"
#include "drw_objects.h"

#include <glm/ext/vector_double2.hpp>

#include <cmath>
#include <print>
#include <string_view>

static auto classify_layer(std::string_view name) 
{
  if (name == "DOOR")      return LayerType::DOOR;
  if (name == "WALL")      return LayerType::WALL;
  if (name == "WINDOW")    return LayerType::WINDOW;
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
  std::println("[Layer] name:`{}`, lineType:{}, tType:{}", data.name, data.lineType, (i32)data.tType);
}

void DRWParser::addLine(const DRW_Line& data)
{
  auto layer_type = classify_layer(data.layer);
  std::println("[Line] name:`{}`", data.layer);
  switch (layer_type)
  {
    case LayerType::WALL:
      wall_segments.push_back(Segment{
        .p1 = glm::dvec2{ data.basePoint.x, data.basePoint.y },
        .p2 = glm::dvec2{ data.secPoint.x, data.secPoint.y },
        .layer = layer_type
      });
      break;
    case LayerType::DOOR:
      door_segments.push_back(Segment{
        .p1 = glm::dvec2{ data.basePoint.x, data.basePoint.y },
        .p2 = glm::dvec2{ data.secPoint.x, data.secPoint.y },
        .layer = layer_type
      });
      break;    
    case LayerType::WINDOW:
      window_segments.push_back(Segment{
        .p1 = glm::dvec2{ data.basePoint.x, data.basePoint.y },
        .p2 = glm::dvec2{ data.secPoint.x, data.secPoint.y },
        .layer = layer_type
      });
      break;
    
    default:
      break;
  }
}

void DRWParser::addPolyline([[maybe_unused]] const DRW_Polyline& data)
{
  // auto layer_name = data.layer;
  auto is_closed = data.flags & 1;
  auto& vertices = data.vertlist;
  std::println("[Polyline] name:`{}`, vertices:{}, is_closed:{}", data.layer, vertices.size(), is_closed);

  //  auto is_closed = data.flags & 1;
  //  auto& vertices = data.vertlist;
  //  if (vertices.size() < 2)
  //    return;
  //
  //  std::ranges::transform(layer_name, layer_name.begin(), [](auto c) { return std::toupper(c); }); 
  //  auto layer_type = classify_layer(layer_name);
  //  if(layer_type == LayerType::WALL || 
  //     layer_type == LayerType::WINDOW || 
  //     layer_type == LayerType::DOOR)
  //  {
  //    std::println("Polyline: layer_name=`{}`, nr_vertices = {}, closed = {}", layer_name, vertices.size(), is_closed);
  //    
  //    // Decompose into individual segments: one per consecutive pair of vertices.
  //    for (auto i = 0u; i < vertices.size() - 1; ++i)
  //    {
  //      auto p0 = vertices.at(i);
  //      auto p1 = vertices.at(i+1);
  //      input_segments.push_back(Segment{
  //        .p1 = glm::dvec2{ p0->basePoint.x, p0->basePoint.y },
  //        .p2 = glm::dvec2{ p1->basePoint.x, p1->basePoint.y },
  //        .layer = layer_type
  //      });
  //    }
  //  
  //    // If closed, add the closing segment from last vertex back to first.
  //    if (is_closed)
  //    {
  //      auto p0 = vertices.front();
  //      auto p_last = vertices.back();
  //      input_segments.push_back(Segment{
  //        .p1 = glm::dvec2{ p_last->basePoint.x, p_last->basePoint.y },
  //        .p2 = glm::dvec2{ p0->basePoint.x, p0->basePoint.y },
  //        .layer = layer_type
  //      });
  //    }   
  //  }
}

void DRWParser::addLWPolyline([[maybe_unused]] const DRW_LWPolyline& data)
{
  // auto layer_name = data.layer;
  auto is_closed = data.flags & 1;
  auto& vertices = data.vertlist;
  std::println("[LWPolyline] name:`{}`, vertices:{}, is_closed:{}", data.layer, vertices.size(), is_closed);

  // auto is_closed = data.flags & 1;
  // auto& vertices = data.vertlist;
  // if (vertices.size() < 2)
  //   return;
  
  // auto layer_type = classify_layer(layer_name);
  // if(layer_type == LayerType::WALL || layer_type == LayerType::WINDOW || layer_type == LayerType::DOOR)
  // {
  //   std::println("LWPolyline: layer_name=`{}`, nr_vertices = {}, closed = {}", layer_name, vertices.size(), is_closed);
  
  //   // Decompose into individual segments: one per consecutive pair of vertices.
  //   for (auto i = 0u; i < vertices.size() - 1; ++i)
  //   {
  //     auto p0 = vertices.at(i);
  //     auto p1 = vertices.at(i+1);
  //     input_segments.push_back(Segment{
  //       .p1 = glm::dvec2{ p0->x, p0->y },
  //       .p2 = glm::dvec2{ p1->x, p1->y },
  //       .layer = layer_type
  //     });
  //   }
  // 
  //   // If closed, add the closing segment from last vertex back to first.
  //   if (is_closed)
  //   {
  //     auto p0 = vertices.front();
  //     auto p_last = vertices.back();
  //     input_segments.push_back(Segment{
  //       .p1 = glm::dvec2{ p_last->x, p_last->y },
  //       .p2 = glm::dvec2{ p0->x, p0->y },
  //       .layer = layer_type
  //     });
  //   }   
  // }
}

void DRWParser::addArc(const DRW_Arc& data)
{
  std::println("[Arc] name:`{}`", data.layer);
  auto layer_type = classify_layer(data.layer);
  if(layer_type == LayerType::DOOR)
  {
    auto center = glm::dvec2{ data.basePoint.x, data.basePoint.y };
    auto radius = data.radious;
    // auto p1 = glm::dvec2{
    //   center.x + radius * std::cos(data.staangle),
    //   center.y + radius * std::sin(data.staangle)
    // };
    auto p2 = glm::dvec2{
      center.x + radius * std::cos(data.endangle),
      center.y + radius * std::sin(data.endangle)
    };
    door_segments.push_back(Segment{ center, p2, layer_type });
  }
}

void DRWParser::addInsert([[maybe_unused]]const DRW_Insert& data)
{  
  std::println("[Insert] name:`{}`, layer:`{}`", data.layer, data.name);
  auto layer_type = classify_layer(data.layer);
  switch (layer_type) 
  {
    case LayerType::DOOR:
    {
      auto angle_rad = data.angle;
      auto door_width = 800.0 * data.xscale;
    
      auto hinge = glm::dvec2{ data.basePoint.x, data.basePoint.y };
      auto tip = glm::dvec2{
        hinge.x + door_width * std::cos(angle_rad),
        hinge.y + door_width * std::sin(angle_rad)
      };
    
      door_segments.push_back(Segment{ hinge, tip, LayerType::DOOR });
      break;
    }

    default:
      break;
  }
}

void DRWParser::addBlock([[maybe_unused]]const DRW_Block& data)
{
  std::println("[Block] name:`{}`", data.layer);
}
