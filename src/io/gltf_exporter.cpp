#include "gltf_exporter.hpp"
#include "../reconstruction.hpp"

#include <stdexcept>
#include <tiny_gltf.h>
#include <glm/common.hpp>


static tinygltf::BufferView create_buffer_view(i32 buffer, 
                                               i32 byte_offset,
                                               i32 byte_length, 
                                               i32 target)
{
  tinygltf::BufferView bv;
  bv.buffer      = buffer;
  bv.byteOffset  = byte_offset;
  bv.byteLength  = byte_length;
  bv.target      = target;
  return bv;
}

static tinygltf::Accessor create_accessor(i32 buffer_view, 
                                          i32 byte_offset,
                                          i32 component_type, 
                                          i64 count,
                                          i32 type)
{
  tinygltf::Accessor acc;
  acc.bufferView    = buffer_view;
  acc.byteOffset    = byte_offset;
  acc.componentType = component_type;
  acc.count         = count;
  acc.type          = type;
  return acc;
}

static tinygltf::Accessor create_accessor(i32 buffer_view, 
                                          i32 byte_offset,
                                          i32 component_type, 
                                          i64 count,
                                          i32 type,
                                          const std::vector<f64>& min_vals,
                                          const std::vector<f64>& max_vals)
{
  auto acc = create_accessor(buffer_view, byte_offset, component_type, count, type);
  acc.minValues = min_vals;
  acc.maxValues = max_vals;
  return acc;
}



void export_to_gltf(const ReconstructionResult& result, 
                    const std::filesystem::path& output_path) 
{
  auto model = tinygltf::Model{};
  model.asset.version = "2.0";
  model.asset.generator = "Plan2Scene";

  // ==========================================
  // Create materials
  // ==========================================
  auto mat_floor = tinygltf::Material{};
  mat_floor.name = "FloorMaterial";
  model.materials.push_back(std::move(mat_floor)); // Index 0

  auto mat_wall = tinygltf::Material{};
  mat_wall.name = "WallMaterial";
  model.materials.push_back(std::move(mat_wall)); // Index 1


  // ==========================================
  // Prepare vertex data (positions, normals, texture coordinates) and index data
  // ==========================================
  auto pos_data = std::vector<f32>{};
  auto nor_data = std::vector<f32>{};
  auto tex_data = std::vector<f32>{};
  pos_data.reserve(result.mesh_vertices.size() * 3);
  nor_data.reserve(result.mesh_vertices.size() * 3);
  tex_data.reserve(result.mesh_vertices.size() * 2);

  auto pos_min = glm::vec3{std::numeric_limits<f32>::max()};
  auto pos_max = glm::vec3{std::numeric_limits<f32>::lowest()};
  for (const auto& v : result.mesh_vertices) 
  {
    pos_data.push_back(v.position.x);
    pos_data.push_back(v.position.y);
    pos_data.push_back(v.position.z);
    pos_min = glm::min(pos_min, v.position);
    pos_max = glm::max(pos_max, v.position);

    nor_data.push_back(v.normal.x);
    nor_data.push_back(v.normal.y);
    nor_data.push_back(v.normal.z);

    tex_data.push_back(v.text_coord.x);
    tex_data.push_back(v.text_coord.y);
  }

  auto pos_byte_len = pos_data.size() * sizeof(f32);
  auto nor_byte_len = nor_data.size() * sizeof(f32);
  auto tex_byte_len = tex_data.size() * sizeof(f32);
  auto idx_byte_len = result.mesh_indices.size() * sizeof(u32);

  auto nor_byte_offset = pos_byte_len;
  auto tex_byte_offset = nor_byte_offset + nor_byte_len;
  auto idx_byte_offset = tex_byte_offset + tex_byte_len;
  auto total_byte_len = idx_byte_offset + idx_byte_len;

  // ==========================================
  // Create buffer and copy data into it
  // ==========================================
  auto buffer = tinygltf::Buffer{};
  buffer.data.resize(total_byte_len);
  
  std::memcpy(buffer.data.data(), pos_data.data(), pos_byte_len);
  std::memcpy(buffer.data.data() + nor_byte_offset, nor_data.data(), nor_byte_len);
  std::memcpy(buffer.data.data() + tex_byte_offset, tex_data.data(), tex_byte_len);
  std::memcpy(buffer.data.data() + idx_byte_offset, result.mesh_indices.data(), idx_byte_len);

  model.buffers.push_back(std::move(buffer));

  // Aggiungiamo le 4 BufferViews (ID: 0=Pos, 1=Nor, 2=Tex, 3=Idx)
  model.bufferViews.push_back(create_buffer_view(0, 0, pos_byte_len, TINYGLTF_TARGET_ARRAY_BUFFER));
  model.bufferViews.push_back(create_buffer_view(0, nor_byte_offset, nor_byte_len, TINYGLTF_TARGET_ARRAY_BUFFER));
  model.bufferViews.push_back(create_buffer_view(0, tex_byte_offset, tex_byte_len, TINYGLTF_TARGET_ARRAY_BUFFER));
  model.bufferViews.push_back(create_buffer_view(0, idx_byte_offset, idx_byte_len, TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER));


  // ==========================================
  // Accessors for positions, normals, texture coordinates, and indices
  // ==========================================

  model.accessors.push_back(create_accessor(0, 0, TINYGLTF_COMPONENT_TYPE_FLOAT, result.mesh_vertices.size(), TINYGLTF_TYPE_VEC3, 
    {pos_min.x, pos_min.y, pos_min.z}, {pos_max.x, pos_max.y, pos_max.z}));
  model.accessors.push_back(create_accessor(1, 0, TINYGLTF_COMPONENT_TYPE_FLOAT, result.mesh_vertices.size(), TINYGLTF_TYPE_VEC3));
  model.accessors.push_back(create_accessor(2, 0, TINYGLTF_COMPONENT_TYPE_FLOAT, result.mesh_vertices.size(), TINYGLTF_TYPE_VEC2));


  // ==========================================
  // Primitive and accessor for indices
  // ==========================================
  auto mesh = tinygltf::Mesh{};
  mesh.name = "RoomModel";
  for (const auto& prim_range : result.primitives) 
  {
    auto acc_idx = tinygltf::Accessor{};
    acc_idx.bufferView = 3;
    acc_idx.byteOffset = prim_range.index_offset * sizeof(u32); 
    acc_idx.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    acc_idx.count = static_cast<i32>(prim_range.index_count);
    acc_idx.type = TINYGLTF_TYPE_SCALAR;
    
    int current_idx_acc_id = static_cast<int>(model.accessors.size());
    model.accessors.push_back(std::move(acc_idx));

    auto primitive = tinygltf::Primitive{};
    primitive.attributes["POSITION"] = 0;
    primitive.attributes["NORMAL"] = 1;
    primitive.attributes["TEXCOORD_0"] = 2;
    primitive.indices = current_idx_acc_id;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;

    if (prim_range.material == MaterialType::Floor) 
      primitive.material = 0; // "FloorMaterial"
    else 
      primitive.material = 1; // "WallMaterial"

    mesh.primitives.push_back(std::move(primitive));
  }

  model.meshes.push_back(std::move(mesh));

  // ==========================================
  // Save the GLTF file
  // ==========================================
  auto node = tinygltf::Node{};
  node.mesh = 0;
  model.nodes.push_back(std::move(node));

  auto scene = tinygltf::Scene{};
  scene.nodes.push_back(0);
  model.scenes.push_back(std::move(scene));
  model.defaultScene = 0;

  auto gltf = tinygltf::TinyGLTF{};
  bool ok = gltf.WriteGltfSceneToFile(&model, output_path.string(), false, false, true, false);
  if (!ok) 
    throw std::runtime_error("Failed to write GLTF file");
}