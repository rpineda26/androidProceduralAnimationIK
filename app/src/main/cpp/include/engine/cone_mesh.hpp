//
// Created by Ralph Dawson Pineda on 6/18/25.
//

#ifndef VULKANANDROID_CONE_MESH_HPP
#define VULKANANDROID_CONE_MESH_HPP
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
namespace ve{
    class ConeMesh{
    public:
        struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        void generateCone(int segments = 8);
    };
}
#endif //VULKANANDROID_CONE_MESH_HPP
