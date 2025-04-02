#pragma once

#include "ve_device.hpp"
#include "skeleton.hpp"
#include "animation_manager.hpp"
#include "buffer.hpp"

#include <android/asset_manager.h>
#include <tiny_gltf.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace ve{

    class VeModel{
    public:
        struct Vertex{
            glm::vec3 position;
            glm::vec3 color;
            glm::vec3 normal;
            glm::vec2 uv;
            glm::vec3 tangent;
            glm::ivec4 jointIndices;
            glm::vec4 jointWeights;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex& other) const{
                return  position == other.position && 
                        color == other.color && 
                        normal == other.normal && 
                        uv == other.uv &&
                        tangent == other.tangent &&
                        jointIndices == other.jointIndices &&
                        jointWeights == other.jointWeights;
            }
        };

        static constexpr int CUBE_MAP_VERTEX_COUNT = 36;
        struct Builder{
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            tinygltf::Model model;
//            void loadModel(const std::string& filePath, AAssetManager *assetManager);
            void loadModelGLTF(const std::string& filePath, AAssetManager *assetManager);
            void loadCubeMap(glm::vec3 cubeVetices[CUBE_MAP_VERTEX_COUNT]);
        };

        VeModel(VeDevice& device, const VeModel::Builder& builder);
        ~VeModel();
        VeModel(const VeModel&) = delete;
        VeModel& operator=(const VeModel&) = delete;

        static std::unique_ptr<VeModel> createModelFromFile(VeDevice& device,AAssetManager *assetManager, const std::string& filePath);
        static std::unique_ptr<VeModel> createCubeMap(VeDevice& device, glm::vec3 cubeVetices[CUBE_MAP_VERTEX_COUNT]);
        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);
        void drawInstanced(VkCommandBuffer commandBuffer, uint32_t instanceCount);
        void updateAnimation(float deltaTime, int frameCounter, int frameIndex);

        AnimationManager& getAnimationManager() { return *animationManager.get(); }
        bool hasAnimationData() const { return hasAnimation; }

        std::unique_ptr<Skeleton> skeleton;
        std::shared_ptr<AnimationManager> animationManager;


    private:
        void createVertexBuffers(const std::vector<Vertex>& vertices);
        void createIndexBuffers(const std::vector<uint32_t>& indices);  
        void loadSkeleton(const tinygltf::Model& model);
        void loadAnimations(const tinygltf::Model& model);
        void loadJoints(int nodeIndex, int parentIndex, const tinygltf::Model& model);
        void extractNodeTransform(const tinygltf::Node& node, Joint& joint);
        glm::mat4 calculateLocalTransform(const Joint& joint);
        void updateJointHierarchy(const tinygltf::Model& model);
        void updateJointWorldMatrices(int jointIndex);
        //attributes
        VeDevice& veDevice;
        //vertex buffer
        std::unique_ptr<VeBuffer> vertexBuffer;
        uint32_t vertexCount;
        //index buffer
        bool hasIndexBuffer{false};
        std::unique_ptr<VeBuffer> indexBuffer;
        uint32_t indexCount;
        //animation data
        bool hasAnimation{false};
    };
}