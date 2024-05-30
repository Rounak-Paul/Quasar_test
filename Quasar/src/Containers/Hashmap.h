#pragma once
// #include <unordered_map>
// #include <Renderer/VulkanBackend/VulkanTypes.inl>
// #include <Defines.h>

// namespace std {
//     template<> struct hash<Quasar::RendererBackend::Vertex> {
//         size_t operator()(Quasar::RendererBackend::Vertex const& vertex) const {
//             return ((hash<glm::vec3>()(vertex.pos) ^
//                    (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
//                    (hash<glm::vec2>()(vertex.texCoord) << 1);
//         }
//     };
// }