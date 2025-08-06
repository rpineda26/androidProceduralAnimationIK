//
// Created by Ralph Dawson Pineda on 6/18/25.
//
#include "cone_mesh.hpp"
namespace ve{
    void ConeMesh::generateCone(int segments) {
        vertices.clear();
        indices.clear();

        // Main pyramid tip (0, 1, 0) - top of bone
        vertices.push_back({{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}});

        // Reverse pyramid tip (0, 0, 0) - pointing toward joint sphere at base
        vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}});

        // Shared base between the two pyramids (at y = 0.25)
        // Use square base for simpler pyramid look
        int pyramidSegments = 4; // Square base
        float baseRadius = 0.05; // Base radius where pyramids meet
        float baseHeight = 0.25f; // Where the pyramids meet

        for (int i = 0; i < pyramidSegments; i++) {
            float angle = 2.0f * M_PI * i / pyramidSegments;
            float x = cos(angle) * baseRadius;
            float z = sin(angle) * baseRadius;

            glm::vec3 pos = {x, baseHeight, z};
            glm::vec3 normal = normalize(glm::vec3(x, 0.0f, z)); // Radial normal

            vertices.push_back({pos, normal});
        }

        // Create main pyramid face vertices (one set per face for flat shading)
        glm::vec3 mainTip = {0.0f, 1.0f, 0.0f};
        for (int i = 0; i < pyramidSegments; i++) {
            int currentBase = 2 + i;
            int nextBase = 2 + ((i + 1) % pyramidSegments);

            // Get base edge positions
            glm::vec3 currentPos = vertices[currentBase].position;
            glm::vec3 nextPos = vertices[nextBase].position;

            // Calculate face normal for main pyramid face
            glm::vec3 edge1 = currentPos - mainTip;
            glm::vec3 edge2 = nextPos - mainTip;
            glm::vec3 faceNormal = normalize(cross(edge2, edge1));

            // Add vertices for this main pyramid face
            vertices.push_back({mainTip, faceNormal});      // tip for this face
            vertices.push_back({currentPos, faceNormal});   // current base for this face
            vertices.push_back({nextPos, faceNormal});      // next base for this face
        }

        // Create reverse pyramid face vertices (one set per face for flat shading)
        glm::vec3 reverseTip = {0.0f, 0.0f, 0.0f};
        for (int i = 0; i < pyramidSegments; i++) {
            int currentBase = 2 + i;
            int nextBase = 2 + ((i + 1) % pyramidSegments);

            // Get base edge positions
            glm::vec3 currentPos = vertices[currentBase].position;
            glm::vec3 nextPos = vertices[nextBase].position;

            // Calculate face normal for reverse pyramid face
            glm::vec3 edge1 = nextPos - reverseTip;  // Note: reversed for downward pyramid
            glm::vec3 edge2 = currentPos - reverseTip;
            glm::vec3 faceNormal = normalize(cross(edge2, edge1));

            // Add vertices for this reverse pyramid face
            vertices.push_back({reverseTip, faceNormal});   // tip for this face
            vertices.push_back({currentPos, faceNormal});   // current base for this face
            vertices.push_back({nextPos, faceNormal});      // next base for this face
        }

        // Generate indices for main pyramid faces - CLOCKWISE ORDER
        int mainPyramidStart = 2 + pyramidSegments; // First main pyramid face vertex

        for (int i = 0; i < pyramidSegments; i++) {
            int faceStart = mainPyramidStart + (i * 3);

            // Triangle for main pyramid face - CLOCKWISE ORDER
            indices.push_back(faceStart);     // tip vertex for this face
            indices.push_back(faceStart + 2); // next base vertex for this face
            indices.push_back(faceStart + 1); // current base vertex for this face
        }

        // Generate indices for reverse pyramid faces - CLOCKWISE ORDER
        int reversePyramidStart = mainPyramidStart + (pyramidSegments * 3); // First reverse pyramid face vertex

        for (int i = 0; i < pyramidSegments; i++) {
            int faceStart = reversePyramidStart + (i * 3);

            // Triangle for reverse pyramid face - CLOCKWISE ORDER
            indices.push_back(faceStart);     // tip vertex for this face
            indices.push_back(faceStart + 1); // current base vertex for this face
            indices.push_back(faceStart + 2); // next base vertex for this face
        }
    }
    std::vector<VkVertexInputBindingDescription> ConeMesh::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> ConeMesh::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        // Location 0: position (vec3)
        attributeDescriptions.push_back({
        0,                                    // location
        0,                                    // binding
        VK_FORMAT_R32G32B32_SFLOAT,          // format (3 floats)
        offsetof(Vertex, position)            // offset
        });

        // Location 1: normal (vec3)
        attributeDescriptions.push_back({
        1,                                    // location
        0,                                    // binding
        VK_FORMAT_R32G32B32_SFLOAT,          // format (3 floats)
        offsetof(Vertex, normal)             // offset
        });

        return attributeDescriptions;
    }

}