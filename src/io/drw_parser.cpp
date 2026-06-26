#include "drw_parser.hpp"
#include "../log.hpp"

#include <glm/ext/vector_double2.hpp>
#include <glm/trigonometric.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>   

#include <print>
#include <string>
#include <string_view>
#include <map>

#include "../geometry.hpp"

// Important: When you read data inside a block, those vertices are in local space. 
// Inside the addInsert call you need to transform those vertices into world spaces. 
// Otherwise, if the primitives are not inside any blocks it means that their vertices are already in world space.

struct DoorBlockInfo 
{
  f64 radius;
};

extern Logger g_logger;

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
  unit_scale = 0.0;
  
  if(!data) 
  { 
    g_logger.push_message({"No header data.", LogLevel::Warning});
    return;
  }
  
  auto it = data->vars.find("$INSUNITS"); 
  if(it == data->vars.end())
  {
    g_logger.push_message({"$INSUNITS not found.", LogLevel::Warning});
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
    g_logger.push_message({"$INSUNITS is not INTEGER.", LogLevel::Warning});
    return;
  }

  switch (units) 
  {
    case 1:  unit_scale = 0.0254;        break;  // Inches
    case 2:  unit_scale = 0.3048;        break;  // Feet
    case 3:  unit_scale = 0.9144;        break;  // Yards
    case 4:  unit_scale = 0.001;         break;  // Millimeters
    case 5:  unit_scale = 0.01;          break;  // Centimeters
    case 6:  unit_scale = 1.0;           break;  // Meters
    case 7:  unit_scale = 1.0e-6;        break;  // Kilometers
    case 21: unit_scale = 0.3048006096;  break;  // US Survey Feet
    default: 
      g_logger.push_message({std::format("Unknown $INSUNITS value: {}", units), LogLevel::Warning});
      break;
  }
  g_logger.push_message({std::format("INSUNITS = {}, scale = {}", units, unit_scale), LogLevel::Text});
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
  
  // =============================
  // WALL
  // =============================
  
  if(layer_type == LayerType::WALL)
  {
    // Break the polyline into segments and add to the walls
    auto segs = to_segments(LayerType::WALL);
    for (auto& seg : segs)
      walls.push_back(seg);
  }
  
  // =============================
  // WINDOW
  // =============================
  
  else if(layer_type == LayerType::WINDOW)
  {
    // Break the polyline into segments and add to the windows
    auto segs = to_segments(LayerType::WINDOW);
    for (auto& seg : segs) 
      windows.push_back(seg);
  }

  // =============================
  // DOOR
  // =============================
  
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
    s_block_door_info[s_current_block_name] = DoorBlockInfo{ data.radious };
    return;
  }
   
  std::println("[Arc] name:`{}`", data.layer);
  auto layer_type = classify_layer(data.layer);

  // =============================
  // DOOR
  // =============================

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
}

void DRWParser::addInsert(const DRW_Insert& data)
{  
  const auto& layer_name = data.layer;
  const auto& block_name = data.name;
  auto angle_rad = data.angle; // in radians 
  auto cos_angle = std::cos(angle_rad);
  auto sin_angle = std::sin(angle_rad);

  std::println("[Insert] layer:`{}`, block:`{}`", layer_name, block_name);
  auto layer_type = classify_layer(data.layer);

  // =============================
  // DOOR
  // =============================

  if(layer_type == LayerType::DOOR)
  {
    auto base_width = 10.0;
    auto it = s_block_door_info.find(block_name);
    if (it != s_block_door_info.end()) 
      base_width = it->second.radius;
    
    auto door_width = base_width * data.xscale;
    auto hinge = glm::dvec2{ data.basePoint.x, data.basePoint.y };
    auto tip = glm::dvec2{
      hinge.x + door_width * cos_angle,
      hinge.y + door_width * sin_angle
    };
    doors.push_back(Segment{ hinge, tip, LayerType::DOOR });
  }
  
  // =============================
  // WINDOW
  // =============================

  else if(layer_type == LayerType::WINDOW)
  {
    auto it = s_block_vertices.find(block_name);
    if (it == s_block_vertices.end()) 
      return;
    
    auto insert_pos = glm::dvec2{ data.basePoint.x, data.basePoint.y };
    auto transform = [&](const glm::dvec2& local) 
    {
      auto scaled = glm::dvec2{ local.x * data.xscale, local.y * data.yscale };
      auto rotated = glm::dvec2{
        scaled.x * cos_angle - scaled.y * sin_angle,
        scaled.x * sin_angle + scaled.y * cos_angle
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