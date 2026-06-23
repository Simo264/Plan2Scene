#include "drw_parser.hpp"

#include <glm/ext/vector_double2.hpp>
#include <glm/trigonometric.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>   

#include <print>
#include <string>
#include <string_view>
#include <map>

#include "../geometry.hpp"

struct DoorBlockInfo 
{
  f64 radius;
  f64 stangle;
  f64 endangle;
};

static auto s_current_block_name = std::string{};
static auto s_is_parsing_block = false;
static auto s_block_vertices = std::map<std::string, std::vector<Segment>>{};
static auto s_block_door_info = std::map<std::string, DoorBlockInfo>{};

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
 
  if(layer_type == LayerType::WALL)
    walls.push_back(segment);
  else if(layer_type == LayerType::WINDOW)    
    windows.push_back(segment);
  else if (layer_type == LayerType::DOOR) 
    doors.push_back(segment);
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
        .layer = layer_type
      });
    }
    return;
  }

  // break polyline into segments
  auto to_segments = [&](LayerType lt) -> std::vector<Segment> 
  {
    auto count = (i32)vertices.size();
    auto num_segments = is_closed ? count : count - 1;
    std::vector<Segment> segs;
    segs.reserve(num_segments);
    for (auto i = 0; i < num_segments; ++i) 
    {
      auto& v1 = vertices[i];
      auto& v2 = vertices[(i + 1) % count];
      segs.push_back(Segment{
        .start = glm::dvec2{ v1->x, v1->y },
        .end   = glm::dvec2{ v2->x, v2->y },
        .layer = lt
      });
    }
    return segs;
  };
  // convert vertices to glm::dvec2
  auto to_points = [&]() -> std::vector<glm::dvec2> 
  {
    std::vector<glm::dvec2> pts;
    pts.reserve(vertices.size());
    for (auto& v : vertices)
      pts.emplace_back(v->x, v->y);
    return pts;
  };

  if(layer_type == LayerType::WALL)
  {
    // Break the polyline into segments and add to the walls
    auto segs = to_segments(LayerType::WALL);
    for (auto& seg : segs)
      walls.push_back(seg);
  }
  else if(layer_type == LayerType::WINDOW)
  {
    // Break the polyline into segments and add to the windows
    auto segs = to_segments(LayerType::WINDOW);
    for (auto& seg : segs) 
      windows.push_back(seg);
  }
  else if(layer_type == LayerType::DOOR)
  {
    // Calculate bb from points
    auto pts = to_points();
    auto bbox = calculate_bbox_2D(pts);
    // We look for the longest side
    auto long_sides = get_long_sides_bbox2d(bbox);
    auto longest_side = long_sides[1];
    longest_side.layer = layer_type;
    doors.push_back(longest_side);
  }
}

void DRWParser::addArc(const DRW_Arc& data)
{
   if (s_is_parsing_block) 
   {
     s_block_door_info[s_current_block_name] = DoorBlockInfo{
        .radius   = data.radious,
        .stangle  = data.staangle,
        .endangle = data.endangle
     };
     return;
   }
   
   std::println("[Arc] name:`{}`", data.layer);
   auto layer_type = classify_layer(data.layer);
   if (layer_type == LayerType::DOOR)
   {
     auto center = glm::dvec2{ data.basePoint.x, data.basePoint.y };
     auto radius = data.radious;
     auto p1 = glm::dvec2{
        center.x + radius * glm::cos(glm::radians(data.staangle)),
        center.y + radius * glm::sin(glm::radians(data.staangle))
     };
     auto p2 = glm::dvec2{
        center.x + radius * glm::cos(glm::radians(data.endangle)),
        center.y + radius * glm::sin(glm::radians(data.endangle))
     };
     doors.push_back(Segment{ p1, p2, layer_type });
   }
   
  //   if (layer_type == LayerType::DOOR)
  //   {
  //     auto center = glm::dvec2{ data.basePoint.x, data.basePoint.y };
  //     auto radius = data.radious;
  //     // auto p1 = glm::dvec2{
  //     //   center.x + radius * std::cos(data.staangle),
  //     //   center.y + radius * std::sin(data.staangle)
  //     // };
  //     auto p2 = glm::dvec2{
  //       center.x + radius * std::cos(data.endangle),
  //       center.y + radius * std::sin(data.endangle)
  //     };
  //     doors.push_back(Segment{ center, p2, layer_type });
  //   }
}

void DRWParser::addInsert(const DRW_Insert& data)
{  
  const auto& layer_name = data.layer;
  const auto& block_name = data.name;
  std::println("[Insert] layer:`{}`, block:`{}`", layer_name, block_name);
  auto layer_type = classify_layer(data.layer);
  if(layer_type == LayerType::DOOR)
  {
    auto base_width = 10.0;
    auto it = s_block_door_info.find(block_name);
    if (it != s_block_door_info.end()) 
    {
      auto& info = it->second;
      auto delta_angle = std::abs(info.endangle - info.stangle);
      // Normalize to [0, 360]
      while (delta_angle > 360.0) 
        delta_angle -= 360.0;
      base_width = info.radius * glm::sin(glm::degrees(delta_angle) / 2.0);
    }
    
    auto angle_rad = data.angle;
    auto door_width = base_width * data.xscale;
    auto hinge = glm::dvec2{ data.basePoint.x, data.basePoint.y };
    auto tip = glm::dvec2{
      hinge.x + door_width * std::cos(angle_rad),
      hinge.y + door_width * std::sin(angle_rad)
    };
    doors.push_back(Segment{ hinge, tip, LayerType::DOOR });
  }
  else if(layer_type == LayerType::WINDOW)
  {
    auto it = s_block_vertices.find(block_name);
    if (it == s_block_vertices.end()) 
      return;
    
    auto insert_pos = glm::dvec2{ data.basePoint.x, data.basePoint.y };
    auto angle_rad = data.angle;
    auto cos_A = std::cos(angle_rad);
    auto sin_A = std::sin(angle_rad);
    auto transform = [&](const glm::dvec2& local) 
    {
      auto scaled = glm::dvec2{ local.x * data.xscale, local.y * data.yscale };
      auto rotated = glm::dvec2{
        scaled.x * cos_A - scaled.y * sin_A,
        scaled.x * sin_A + scaled.y * cos_A
      };
      return glm::dvec2{ rotated.x + insert_pos.x, rotated.y + insert_pos.y };
    };
    
    for (const auto& local_segment : it->second) 
    {    
      auto start = transform(local_segment.start);
      auto end = transform(local_segment.end);
      windows.push_back(Segment{ start, end, LayerType::WINDOW });
    }
  }

  
  // switch (layer_type)
  // {
  //   case LayerType::DOOR:
  //   {
  //     auto base_width = 10.0;
  //     auto it = s_block_door_widths.find(block_name);
  //     if (it != s_block_door_widths.end()) 
  //       base_width = it->second;

  //     auto angle_rad = data.angle; 
  //     auto door_width = base_width * data.xscale; 
  //     auto hinge = glm::dvec2{ data.basePoint.x, data.basePoint.y }; 
  //     auto tip = glm::dvec2{ 
  //       hinge.x + door_width * std::cos(angle_rad), 
  //       hinge.y + door_width * std::sin(angle_rad) 
  //     };
  //     doors.push_back(Segment{ hinge, tip, LayerType::DOOR });
  //     break;
  //   }
  //   case LayerType::WINDOW: 
  //   {
  //     auto it = s_block_vertices.find(block_name);
  //     if (it == s_block_vertices.end()) 
  //       break;

  //     auto insert_pos = glm::dvec2{ data.basePoint.x, data.basePoint.y };
  //     auto angle_rad  = data.angle;
  //     auto cos_A = std::cos(angle_rad);
  //     auto sin_A = std::sin(angle_rad);
  //     auto transform = [&](const glm::dvec2& local) {
  //       auto scaled = glm::dvec2{ local.x * data.xscale, local.y * data.yscale };
  //       auto rotated = glm::dvec2{
  //           scaled.x * cos_A - scaled.y * sin_A,
  //           scaled.x * sin_A + scaled.y * cos_A
  //       };
  //       return glm::dvec2{ rotated.x + insert_pos.x, rotated.y + insert_pos.y };
  //     };

  //     for (const auto& local_segment : it->second) 
  //     {
  //       auto start = transform(local_segment.start);
  //       auto end   = transform(local_segment.end);
  //       windows.push_back(Segment{ start, end, LayerType::WINDOW });
  //     }
  //     break;
  //   }

  //   default:
  //     break;
  // }
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