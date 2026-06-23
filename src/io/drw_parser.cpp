#include "drw_parser.hpp"
#include "drw_objects.h"

#include <glm/ext/vector_double2.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>   

#include <print>
#include <string>
#include <string_view>
#include <map>

static auto s_current_block_name = std::string{};
static auto s_is_parsing_block = false;
// those points are in local coordinates of the block
static auto s_block_vertices = std::map<std::string, std::vector<Segment>>{};
static auto s_block_door_widths = std::map<std::string, f64>{};

static auto classify_layer(std::string_view name) 
{
  if (name.contains("DOOR"))      return LayerType::DOOR;
  if (name.contains("WALL"))      return LayerType::WALL;
  if (name.contains("WINDOW"))    return LayerType::WINDOW;
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
  //std::println("[Layer] name:`{}`", data.name);
}

void DRWParser::addLine(const DRW_Line& data)
{
  auto layer_type = classify_layer(data.layer);
  std::println("[Line] name:`{}`", data.layer);
  auto segment = Segment{
    .start = glm::dvec2{ data.basePoint.x, data.basePoint.y },
    .end = glm::dvec2{ data.secPoint.x, data.secPoint.y },
    .layer = layer_type
  };

  if (s_is_parsing_block) 
  {
    // in local coordinates
    s_block_vertices[s_current_block_name].push_back(segment);
    return;
  }


  
  switch (layer_type)
  {
    case LayerType::WALL:
      walls.push_back(segment);
      break;
    case LayerType::WINDOW:
      windows.push_back(segment);
      break;
    
    default:
      break;
  }
}

void DRWParser::addPolyline(const DRW_Polyline& data)
{
  auto is_closed = data.flags & 1;
  auto& vertices = data.vertlist;
  std::println("[Polyline] name:`{}`, vertices:{}, is_closed:{}", data.layer, vertices.size(), is_closed);
}

void DRWParser::addLWPolyline(const DRW_LWPolyline& data)
{
  auto layer_type = classify_layer(data.layer);
  auto is_closed = data.flags & 1;
  auto& vertices = data.vertlist;
  std::println("[LWPolyline] name:`{}`, vertices:{}, is_closed:{}", data.layer, vertices.size(), is_closed);

  if(vertices.empty())
    return;

  if (s_is_parsing_block) 
  {
    auto count = (i32) vertices.size();
    auto num_segments = is_closed ? count : count - 1;
    for (auto i = 0; i < num_segments; ++i) 
    {
      auto& v1 = vertices[i];
      auto& v2 = vertices[(i + 1) % count];
      s_block_vertices[s_current_block_name].push_back(Segment{
        .start = glm::dvec2{ v1->x, v1->y },
        .end   = glm::dvec2{ v2->x, v2->y },
        .layer = LayerType::NONE
      });
    }
    return;
  }


  if (!is_closed)
    return;
  
  switch (layer_type)
  {
    case LayerType::DOOR:
    {
      // We look for the longest side
      
      auto start = glm::dvec2{};
      auto end = glm::dvec2{};
      auto max_len_sq = -1.0;
      for (auto i = 0; i < 4; ++i)
      {
        auto v1 = glm::dvec2(vertices[i]->x, vertices[i]->y);
        auto v2 = glm::dvec2(vertices[(i + 1) % 4]->x, vertices[(i + 1) % 4]->y);
        auto len_sq = glm::length2(v2 - v1);
        if (len_sq > max_len_sq) 
        {
          max_len_sq = len_sq;
          start = v1;
          end = v2;
        }
      }
      doors.push_back(Segment{ start, end, layer_type });
      break;
    }
  
    default:
      break;
  }

}

void DRWParser::addArc(const DRW_Arc& data)
{
  if (s_is_parsing_block) 
  {
    s_block_door_widths[s_current_block_name] = data.radious;
    return;
  }

  std::println("[Arc] name:`{}`", data.layer);
  auto layer_type = classify_layer(data.layer);
  if (layer_type == LayerType::DOOR)
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
    doors.push_back(Segment{ center, p2, layer_type });
  }
}

void DRWParser::addInsert(const DRW_Insert& data)
{  
  const auto& layer_name = data.layer;
  const auto& block_name = data.name;
  std::println("[Insert] layer:`{}`, block:`{}`", layer_name, block_name);
  auto layer_type = classify_layer(data.layer);
  switch (layer_type)
  {
    case LayerType::DOOR:
    {
      auto base_width = 10.0;
      auto it = s_block_door_widths.find(block_name);
      if (it != s_block_door_widths.end()) 
        base_width = it->second;

      auto angle_rad = data.angle; 
      auto door_width = base_width * data.xscale; 
      auto hinge = glm::dvec2{ data.basePoint.x, data.basePoint.y }; 
      auto tip = glm::dvec2{ 
        hinge.x + door_width * std::cos(angle_rad), 
        hinge.y + door_width * std::sin(angle_rad) 
      };
      doors.push_back(Segment{ hinge, tip, LayerType::DOOR });
      break;
    }
    case LayerType::WINDOW: 
    {
      auto it = s_block_vertices.find(block_name);
      if (it == s_block_vertices.end()) 
        break;

      auto insert_pos = glm::dvec2{ data.basePoint.x, data.basePoint.y };
      auto angle_rad  = data.angle;
      auto cos_A = std::cos(angle_rad);
      auto sin_A = std::sin(angle_rad);

      auto transform = [&](const glm::dvec2& local) {
          auto scaled = glm::dvec2{ local.x * data.xscale, local.y * data.yscale };
          auto rotated = glm::dvec2{
              scaled.x * cos_A - scaled.y * sin_A,
              scaled.x * sin_A + scaled.y * cos_A
          };
          return glm::dvec2{ rotated.x + insert_pos.x, rotated.y + insert_pos.y };
      };

      for (const auto& local_segment : it->second) 
      {
        auto world_start = transform(local_segment.start);
        auto world_end   = transform(local_segment.end);
        windows.push_back(Segment{ world_start, world_end, LayerType::WINDOW });
      }
      break;
    }

    default:
      break;
  }
}

void DRWParser::addBlock(const DRW_Block& data)
{
  std::println("[addBlock] name={}", data.name);
  if(data.name.contains("BLOCK_DOOR") || data.name.contains("BLOCK_WINDOW"))
  {
    s_current_block_name = data.name;
    s_is_parsing_block = true;
    s_block_vertices[s_current_block_name].clear();
  }
}

void DRWParser::endBlock()
{
  std::println("[endBlock] name={}", s_current_block_name);
  s_current_block_name.clear();
  s_is_parsing_block = false;
}