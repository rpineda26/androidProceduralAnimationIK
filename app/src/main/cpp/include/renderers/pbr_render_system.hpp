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
    class PbrRenderSystem{
        public:
            PbrRenderSystem(VeDevice& device, AAssetManager *assetManager, VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
            ~PbrRenderSystem();
            PbrRenderSystem(const PbrRenderSystem&) = delete;
            PbrRenderSystem& operator=(const PbrRenderSystem&) = delete;
            void renderGameObjects( FrameInfo& frameInfo, /*VkDescriptorSet shadowDescriptorSet,*/const std::vector<VkDescriptorSet>& descriptorSets);

        private:
            void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
            void createPipeline(AAssetManager *assetManager, VkRenderPass renderPass);

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> vePipeline;
            VkPipelineLayout pipelineLayout{};
    };
}