#pragma once

#include "ve_pipeline.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include "frame_info.hpp"
#include "cone_mesh.hpp"
#include "ve_descriptors.hpp"

#include <android/asset_manager.h>
#include <memory>
#include <vector>
namespace ve {
    class SkeletonSystem{
        public:
            SkeletonSystem(VeDevice& device, AAssetManager *assetManager, VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, float aspect);
            ~SkeletonSystem();
            SkeletonSystem(const SkeletonSystem&) = delete;
            SkeletonSystem& operator=(const SkeletonSystem&) = delete;
            void renderJoints( FrameInfo& frameInfo, const std::vector<VkDescriptorSet>& descriptorSets);
            void renderJointConnections(FrameInfo& frameInfo, const std::vector<VkDescriptorSet>& descriptorSets);
        private:
            void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
            void createPipeline(AAssetManager *assetManager, VkRenderPass renderPass);
            void initializeConeGeometry();
            glm::mat4 createConeTransform(const glm::vec3& start, const glm::vec3& end);

            VeDevice& veDevice;
            //pipeline
            std::unique_ptr<VePipeline> jointSpheresPipeline;
            std::unique_ptr<VePipeline> boneLinesPipeline;
            VkPipelineLayout jointSpheresPipelineLayout{};
            VkPipelineLayout boneLinesPipelineLayout{};
            //fix perspective when horizontal orientation
            float aspectRatio;
            //cone geometry
            std::unique_ptr<ConeMesh> coneMesh;
            std::unique_ptr<VeBuffer> coneVertexBuffer;
            std::unique_ptr<VeBuffer> coneIndexBuffer;
            //cone descriptor set
            std::unique_ptr<VeDescriptorSetLayout> coneDescriptorSetLayout;
            VkDescriptorSet coneDescriptorSet;

    };
}