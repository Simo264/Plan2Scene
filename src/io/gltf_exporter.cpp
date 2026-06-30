#include "gltf_exporter.hpp"

#include <stdexcept>
#include <tiny_gltf.h>
#include <glm/common.hpp>

void export_to_gltf(const std::vector<Vertex_PNT>& vertices, 
                    const std::vector<u32>& indices, 
                    const std::filesystem::path& output_path) 
{
  auto model = tinygltf::Model{};
  model.asset.version = "2.0";
  model.asset.generator = "Plan2Scene";

  // Prepara i vettori di dati
  auto pos_data = std::vector<f32>{};
  auto nor_data = std::vector<f32>{};
  auto tex_data = std::vector<f32>{}; // Nuovo vettore per UV
  auto idx_data = std::vector<u32>{};

  pos_data.reserve(vertices.size() * 3);
  nor_data.reserve(vertices.size() * 3);
  tex_data.reserve(vertices.size() * 2); // 2 float per UV
  idx_data.reserve(indices.size());

  auto pos_min = glm::vec3{std::numeric_limits<f32>::max()};
  auto pos_max = glm::vec3{std::numeric_limits<f32>::lowest()};

  // Popola i dati
  for (const auto& v : vertices) 
  {
    // Posizione
    pos_data.push_back(v.position.x);
    pos_data.push_back(v.position.y);
    pos_data.push_back(v.position.z);
    pos_min = glm::min(pos_min, v.position);
    pos_max = glm::max(pos_max, v.position);

    // Normale
    nor_data.push_back(v.normal.x);
    nor_data.push_back(v.normal.y);
    nor_data.push_back(v.normal.z);

    // Texture Coordinate (UV)
    tex_data.push_back(v.text_coord.x);
    tex_data.push_back(v.text_coord.y);
  }

  for (const auto idx : indices) 
    idx_data.push_back(idx);
  
  // Calcola le lunghezze e gli offset in byte
  auto pos_byte_len = static_cast<size_t>(pos_data.size()) * sizeof(f32);
  auto nor_byte_len = static_cast<size_t>(nor_data.size()) * sizeof(f32);
  auto tex_byte_len = static_cast<size_t>(tex_data.size()) * sizeof(f32); // Nuova lunghezza
  auto idx_byte_len = static_cast<size_t>(idx_data.size()) * sizeof(u32);

  // Ordine: Pos -> Norm -> Tex -> Indici
  auto nor_byte_offset = pos_byte_len;
  auto tex_byte_offset = nor_byte_offset + nor_byte_len; // Nuovo offset
  auto idx_byte_offset = tex_byte_offset + tex_byte_len; // Nuovo offset
  auto total_byte_len = idx_byte_offset + idx_byte_len;

  // Crea il Buffer
  auto buffer = tinygltf::Buffer{};
  buffer.data.resize(total_byte_len);
  
  std::memcpy(buffer.data.data(), pos_data.data(), pos_byte_len);
  std::memcpy(buffer.data.data() + nor_byte_offset, nor_data.data(), nor_byte_len);
  std::memcpy(buffer.data.data() + tex_byte_offset, tex_data.data(), tex_byte_len); // Copia UV
  std::memcpy(buffer.data.data() + idx_byte_offset, idx_data.data(), idx_byte_len);

  model.buffers.push_back(std::move(buffer));

  // Crea i BufferViews
  // View Posizione (Index 0)
  auto bv_pos = tinygltf::BufferView{};
  bv_pos.buffer = 0;
  bv_pos.byteOffset = 0;
  bv_pos.byteLength = pos_byte_len;
  bv_pos.target = TINYGLTF_TARGET_ARRAY_BUFFER;
  model.bufferViews.push_back(std::move(bv_pos));

  // View Normale (Index 1)
  auto bv_nor = tinygltf::BufferView{};
  bv_nor.buffer = 0;
  bv_nor.byteOffset = nor_byte_offset;
  bv_nor.byteLength = nor_byte_len;
  bv_nor.target = TINYGLTF_TARGET_ARRAY_BUFFER;
  model.bufferViews.push_back(std::move(bv_nor));

  // View Texture (Index 2)
  auto bv_tex = tinygltf::BufferView{};
  bv_tex.buffer = 0;
  bv_tex.byteOffset = tex_byte_offset;
  bv_tex.byteLength = tex_byte_len;
  bv_tex.target = TINYGLTF_TARGET_ARRAY_BUFFER;
  model.bufferViews.push_back(std::move(bv_tex));

  // View Indici (Index 3)
  auto bv_idx = tinygltf::BufferView{};
  bv_idx.buffer = 0;
  bv_idx.byteOffset = idx_byte_offset;
  bv_idx.byteLength = idx_byte_len;
  bv_idx.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
  model.bufferViews.push_back(std::move(bv_idx));

  // Crea gli Accessors
  // Accessor Posizione (Index 0)
  auto acc_pos = tinygltf::Accessor{};
  acc_pos.bufferView = 0;
  acc_pos.byteOffset = 0;
  acc_pos.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
  acc_pos.count = static_cast<i32>(vertices.size());
  acc_pos.type = TINYGLTF_TYPE_VEC3;
  acc_pos.minValues = {pos_min.x, pos_min.y, pos_min.z};
  acc_pos.maxValues = {pos_max.x, pos_max.y, pos_max.z};
  model.accessors.push_back(std::move(acc_pos));

  // Accessor Normale (Index 1)
  auto acc_nor = tinygltf::Accessor{};
  acc_nor.bufferView = 1;
  acc_nor.byteOffset = 0;
  acc_nor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
  acc_nor.count = static_cast<i32>(vertices.size());
  acc_nor.type = TINYGLTF_TYPE_VEC3;
  model.accessors.push_back(std::move(acc_nor));

  // Accessor Texture (Index 2)
  auto acc_tex = tinygltf::Accessor{};
  acc_tex.bufferView = 2; // Corrisponde a bv_tex
  acc_tex.byteOffset = 0;
  acc_tex.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
  acc_tex.count = static_cast<i32>(vertices.size());
  acc_tex.type = TINYGLTF_TYPE_VEC2; // Tipo VEC2 per UV
  model.accessors.push_back(std::move(acc_tex));

  // Accessor Indici (Index 3)
  auto acc_idx = tinygltf::Accessor{};
  acc_idx.bufferView = 3; // Corrisponde a bv_idx
  acc_idx.byteOffset = 0;
  acc_idx.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
  acc_idx.count = static_cast<i32>(indices.size());
  acc_idx.type = TINYGLTF_TYPE_SCALAR;
  model.accessors.push_back(std::move(acc_idx));

  // 7. Configura il Primitive
  auto primitive = tinygltf::Primitive{};
  primitive.attributes["POSITION"] = 0;
  primitive.attributes["NORMAL"] = 1;
  primitive.attributes["TEXCOORD_0"] = 2; // Mappa TEXCOORD_0 all'accessor ID 2
  
  primitive.indices = 3; // Mappa indici all'accessor ID 3
  primitive.mode = TINYGLTF_MODE_TRIANGLES;

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
  // Embedding dei binari nel file JSON per semplicità, o esterno se preferisci
  bool ok = gltf.WriteGltfSceneToFile(&model, output_path, false, false, true, false);
  if (!ok) 
    throw std::runtime_error("Failed to write GLTF file");
}