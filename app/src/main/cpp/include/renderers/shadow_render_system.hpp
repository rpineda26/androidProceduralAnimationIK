//
// Created by Ralph Dawson Pineda on 7/3/25.
//

#ifndef VULKANANDROID_SHADOW_RENDER_SYSTEM_HPP
#define VULKANANDROID_SHADOW_RENDER_SYSTEM_HPP
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <android/asset_manager.h>

#include "ve_device.hpp"
#include "ve_pipeline.hpp"
#include "frame_info.hpp"
namespace ve{


    class ShadowRenderSystem {
    public:
        static constexpr int SHADOW_MAP_SIZE = 512;

        struct ShadowPushConstants {
            glm::mat4 mvpMatrix;  // 64 bytes
            glm::vec3 lightPos;   // 12 bytes
            bool isAnimated;      // 1 byte
        };

        ShadowRenderSystem(VeDevice& device, AAssetManager* assetManager, VkRenderPass shadowRenderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
        ~ShadowRenderSystem();
        ShadowRenderSystem(const ShadowRenderSystem&) = delete;
        ShadowRenderSystem& operator=(const ShadowRenderSystem&) = delete;

        void renderObjectsForShadowFace(VkCommandBuffer commandBuffer,
                                        const FrameInfo& frameInfo,
                                        int lightIndex,
                                        int faceIndex,
                                        const glm::mat4& viewProjMatrix,
                                        const glm::vec3& lightPos,
                                        float lightRadius);

        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; };
        VkPipeline getPipeline() const { return vePipeline->getPipeline(); };

    private:
        void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
        void createPipeline(AAssetManager* assetManager, VkRenderPass renderPass);

        VeDevice& veDevice;
        std::unique_ptr<VePipeline> vePipeline;
        VkPipelineLayout pipelineLayout;
    };
}
#endif //VULKANANDROID_SHADOW_RENDER_SYSTEM_HPP
