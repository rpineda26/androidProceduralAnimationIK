#pragma once

#include "ve_pipeline.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include "frame_info.hpp"

#include <android/asset_manager.h>
#include <memory>
#include <vector>
namespace ve {
    class SkeletonSystem{
        public:
            SkeletonSystem(VeDevice& device, AAssetManager *assetManager, VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
            ~SkeletonSystem();
            SkeletonSystem(const SkeletonSystem&) = delete;
            SkeletonSystem& operator=(const SkeletonSystem&) = delete;
            void renderJoints( FrameInfo& frameInfo, const std::vector<VkDescriptorSet>& descriptorSets);
            void renderJointConnections(FrameInfo& frameInfo, const std::vector<VkDescriptorSet>& descriptorSets);
        private:
            void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
            void createPipeline(AAssetManager *assetManager, VkRenderPass renderPass);

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> jointSpheresPipeline;
            std::unique_ptr<VePipeline> boneLinesPipeline;
            VkPipelineLayout jointSpheresPipelineLayout{};
            VkPipelineLayout boneLinesPipelineLayout{};

    };
}