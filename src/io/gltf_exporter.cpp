#include "gltf_exporter.hpp"

#include <stdexcept>
#include <tiny_gltf.h>
#include <glm/common.hpp>

void export_to_gltf(const std::vector<Vertex_PN>& vertices,
                    const std::vector<u32>& indices,
                    const std::filesystem::path& output_path)
{
  auto model = tinygltf::Model{};
  model.asset.version   = "2.0";
  model.asset.generator = "Plan2Scene";

  // Pack all data into a single buffer:
  // [positions | normals | indices]
  auto pos_data  = std::vector<f32>{};
  auto nor_data  = std::vector<f32>{};
  auto idx_data  = std::vector<u32>{};

  pos_data.reserve(vertices.size() * 3);
  nor_data.reserve(vertices.size() * 3);
  idx_data.reserve(indices.size());

  auto pos_min = glm::vec3{std::numeric_limits<f32>::max()};
  auto pos_max = glm::vec3{std::numeric_limits<f32>::lowest()};
  for (const auto& v : vertices) 
  {
    pos_data.push_back(v.position.x);
    pos_data.push_back(v.position.y);
    pos_data.push_back(v.position.z);
    pos_min = glm::min(pos_min, v.position);
    pos_max = glm::max(pos_max, v.position);

    nor_data.push_back(v.normal.x);
    nor_data.push_back(v.normal.y);
    nor_data.push_back(v.normal.z);
  }
  
  for (const auto idx : indices)
    idx_data.push_back(idx);

  // Byte sizes
  auto pos_byte_len = pos_data.size() * sizeof(f32);
  auto nor_byte_len = nor_data.size() * sizeof(f32);
  auto idx_byte_len = idx_data.size() * sizeof(u32);
  // Offsets inside the buffer (indices must be 4-byte aligned)
  auto nor_byte_offset = pos_byte_len;
  auto idx_byte_offset = nor_byte_offset + nor_byte_len;
  auto total_byte_len  = idx_byte_offset + idx_byte_len;

  // Buffer views  (one per data stream)
  auto buffer = tinygltf::Buffer{};
  buffer.data.resize(total_byte_len);
  std::memcpy(buffer.data.data(),                    pos_data.data(), pos_byte_len);
  std::memcpy(buffer.data.data() + nor_byte_offset,  nor_data.data(), nor_byte_len);
  std::memcpy(buffer.data.data() + idx_byte_offset,  idx_data.data(), idx_byte_len);
  model.buffers.push_back(std::move(buffer));

  // BufferView: positions
  auto bv_pos = tinygltf::BufferView{};
  bv_pos.buffer     = 0;
  bv_pos.byteOffset = 0;
  bv_pos.byteLength = pos_byte_len;
  bv_pos.target     = TINYGLTF_TARGET_ARRAY_BUFFER;
  model.bufferViews.push_back(std::move(bv_pos));
  // BufferView: normals
  auto bv_nor = tinygltf::BufferView{};
  bv_nor.buffer     = 0;
  bv_nor.byteOffset = nor_byte_offset;
  bv_nor.byteLength = nor_byte_len;
  bv_nor.target     = TINYGLTF_TARGET_ARRAY_BUFFER;
  model.bufferViews.push_back(std::move(bv_nor));
  // BufferView: indices
  auto bv_idx = tinygltf::BufferView{};
  bv_idx.buffer     = 0;
  bv_idx.byteOffset = idx_byte_offset;
  bv_idx.byteLength = idx_byte_len;
  bv_idx.target     = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
  model.bufferViews.push_back(std::move(bv_idx));

  // Accessor: positions
  auto acc_pos = tinygltf::Accessor{};
  acc_pos.bufferView    = 0;
  acc_pos.byteOffset    = 0;
  acc_pos.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
  acc_pos.count         = static_cast<i32>(vertices.size());
  acc_pos.type          = TINYGLTF_TYPE_VEC3;
  acc_pos.minValues     = {pos_min.x, pos_min.y, pos_min.z};
  acc_pos.maxValues     = {pos_max.x, pos_max.y, pos_max.z};
  model.accessors.push_back(std::move(acc_pos));
  // Accessor: normals
  auto acc_nor = tinygltf::Accessor{};
  acc_nor.bufferView    = 1;
  acc_nor.byteOffset    = 0;
  acc_nor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
  acc_nor.count         = static_cast<i32>(vertices.size());
  acc_nor.type          = TINYGLTF_TYPE_VEC3;
  model.accessors.push_back(std::move(acc_nor));
  // Accessor: indices
  auto acc_idx = tinygltf::Accessor{};
  acc_idx.bufferView    = 2;
  acc_idx.byteOffset    = 0;
  acc_idx.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
  acc_idx.count         = static_cast<i32>(indices.size());
  acc_idx.type          = TINYGLTF_TYPE_SCALAR;
  model.accessors.push_back(std::move(acc_idx));

  auto primitive = tinygltf::Primitive{};
  primitive.attributes["POSITION"] = 0;  // accessor 0
  primitive.attributes["NORMAL"]   = 1;  // accessor 1
  primitive.indices                 = 2;  // accessor 2
  primitive.mode                    = TINYGLTF_MODE_TRIANGLES;

  // Primitive, mesh, node, scene
  auto mesh = tinygltf::Mesh{};
  mesh.name = "Room";
  mesh.primitives.push_back(std::move(primitive));
  model.meshes.push_back(std::move(mesh));
  auto node = tinygltf::Node{};
  node.mesh = 0;
  model.nodes.push_back(std::move(node));
  auto scene = tinygltf::Scene{};
  scene.nodes.push_back(0);
  model.scenes.push_back(std::move(scene));
  model.defaultScene = 0;

  auto gltf = tinygltf::TinyGLTF{};
  auto ok = gltf.WriteGltfSceneToFile(&model, 
                                      output_path,
                                      false, 
                                      false,
                                      true, 
                                      false);
  if (!ok)
    throw std::runtime_error(std::format("TinyGLTF: failed to write {}", output_path.string()));
}