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
    class CubeMapRenderSystem{
        public:
            CubeMapRenderSystem(VeDevice& device, AAssetManager *assetManager, VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
            ~CubeMapRenderSystem();
            CubeMapRenderSystem(const CubeMapRenderSystem&) = delete;
            CubeMapRenderSystem& operator=(const CubeMapRenderSystem&) = delete;
            void renderGameObjects( FrameInfo& frameInfo);

        private:
            void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
            void createPipeline(AAssetManager *assetManager, VkRenderPass renderPass);

            VeDevice& veDevice;
            std::unique_ptr<VePipeline> vePipeline;
            VkPipelineLayout pipelineLayout;
    };
}