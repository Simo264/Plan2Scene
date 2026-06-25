// void MeshVisualizer::show_mesh_props() 
// {
//   ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);
//   if (!ImGui::Begin("Mesh Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
//   {
//     ImGui::End();
//     return;
//   }
// 
//   if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) 
//   {
//     auto changed = false;
//     changed |= ImGui::DragFloat3("Position", &m_mesh_transform.position.x, 0.1f);
//     
//     auto rot_deg = glm::degrees(m_mesh_transform.rotation);
//     if (ImGui::DragFloat3("Rotation", &rot_deg.x, 0.5f, -180.0f, 180.0f, "%.1f°"))
//     {
//       m_mesh_transform.rotation = glm::radians(rot_deg);
//       changed = true;
//     }
//     
//     changed |= ImGui::DragFloat3("Scale", &m_mesh_transform.scale.x, 0.05f, 0.001f, 100.0f);
//     if (changed) 
//       m_mesh_transform.update_tranformation();
//   }
// 
//   ImGui::Spacing();
// 
//   if (ImGui::CollapsingHeader("Geometry Data", ImGuiTreeNodeFlags_DefaultOpen)) 
//   {
//     ImGui::Columns(2, "mesh_stats", false);
//     ImGui::SetColumnWidth(0, 120.0f);
//     ImGui::Text("Vertices:");   ImGui::NextColumn(); ImGui::Text("%u", m_mesh->nr_vertices()); ImGui::NextColumn();
//     ImGui::Text("Indices:");    ImGui::NextColumn(); ImGui::Text("%u", m_mesh->nr_indices());  ImGui::NextColumn();
//     ImGui::Columns(1);
//   }
// 
//   ImGui::Spacing();
// 
//   if (ImGui::CollapsingHeader("Video memory (VRAM)")) 
//   {
//     auto vbo_size = m_mesh->nr_vertices() * sizeof(Vertex_PN); // 24 bytes per vertice
//     auto ibo_size = m_mesh->nr_indices() * sizeof(u32);     // 4 bytes per indice
//     auto total_kb = (vbo_size + ibo_size) / 1024.0f;
// 
//     ImGui::Text("VBO Size:"); 
//     ImGui::SameLine(); ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%.2f KB", vbo_size / 1024.0f);
//     
//     ImGui::Text("IBO Size:"); 
//     ImGui::SameLine(); ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%.2f KB", ibo_size / 1024.0f);
//     
//     ImGui::Separator();
//     ImGui::Text("Total:"); 
//     ImGui::SameLine(); ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%.2f KB", total_kb);
//   }
// 
//   ImGui::End();
// }

