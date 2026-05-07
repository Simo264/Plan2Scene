#include "gltf_exporter.hpp"

#include <tiny_gltf.h>
#include <glm/common.hpp>

void export_to_gltf(const std::vector<Vertex>& vertices,
                    const std::filesystem::path& output_path)
{
  auto model = tinygltf::Model{};
  model.asset.version   = "2.0";
  model.asset.generator = "Plan2Scene";

  auto position_data = std::vector<float>{};
  position_data.reserve(vertices.size() * 3);

  auto pos_min = glm::vec3{ std::numeric_limits<float>::max() };
  auto pos_max = glm::vec3{ std::numeric_limits<float>::lowest() };
  for (const auto& v : vertices) 
  {
    position_data.push_back(v.position.x);
    position_data.push_back(v.position.y);
    position_data.push_back(v.position.z);
    pos_min = glm::min(pos_min, v.position);
    pos_max = glm::max(pos_max, v.position);
  }

  auto buffer = tinygltf::Buffer{};
  auto byte_length = position_data.size() * sizeof(float);
  buffer.data.resize(byte_length);
  std::memcpy(buffer.data.data(), position_data.data(), byte_length);
  model.buffers.push_back(std::move(buffer));

  auto buffer_view = tinygltf::BufferView{};
  buffer_view.buffer     = 0;
  buffer_view.byteOffset = 0;
  buffer_view.byteLength = byte_length;
  buffer_view.target     = TINYGLTF_TARGET_ARRAY_BUFFER;
  model.bufferViews.push_back(std::move(buffer_view));

  auto accessor = tinygltf::Accessor{};
  accessor.bufferView    = 0;
  accessor.byteOffset    = 0;
  accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
  accessor.count         = static_cast<int>(vertices.size());
  accessor.type          = TINYGLTF_TYPE_VEC3;
  accessor.minValues     = { pos_min.x, pos_min.y, pos_min.z };
  accessor.maxValues     = { pos_max.x, pos_max.y, pos_max.z };
  model.accessors.push_back(std::move(accessor));

  auto primitive = tinygltf::Primitive{};
  primitive.attributes["POSITION"] = 0;           // accessor index
  primitive.mode                   = TINYGLTF_MODE_TRIANGLES;
  // No indices => non-indexed draw (every 3 consecutive vertices = 1 triangle)
   
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