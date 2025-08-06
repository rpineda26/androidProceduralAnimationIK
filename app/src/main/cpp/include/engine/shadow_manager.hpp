//
// Created by Ralph Dawson Pineda on 7/3/25.
//

#ifndef VULKANANDROID_SHADOW_MANAGER_HPP
#define VULKANANDROID_SHADOW_MANAGER_HPP

#include "ve_device.hpp"
#include "frame_info.hpp"
#include "shadow_render_system.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

namespace ve{
    class ShadowCubeMap {
    public:
        static constexpr int SHADOW_MAP_SIZE = 512;  // Android-optimized size
        static constexpr int CUBE_FACES = 6;

        ShadowCubeMap(VeDevice& device);
        ~ShadowCubeMap();

        // Create all Vulkan resources for this cube map
        bool create();
        void cleanup();

        // Record commands to render shadow map for this light
        void recordShadowPass(VkCommandBuffer commandBuffer,
                              const glm::vec3& lightPos,
                              VkPipelineLayout shadowPipelineLayout,
                              VkPipeline shadowPipeline,
                              std::function<void(VkCommandBuffer, int, const glm::mat4&)> drawScene);

        // Getters for binding to main shader
        VkImageView getCubeImageView() const { return cubeImageView; }
        VkSampler getSampler() const { return sampler; }
        VkRenderPass getRenderPass() const { return renderPass; }

        // Utility functions
        std::array<glm::mat4, 6> getViewMatrices(const glm::vec3& lightPos) const;
        glm::mat4 getProjectionMatrix() const;

    private:
        VeDevice& device;

        // Vulkan resources for the cube map
        VkImage depthCubeImage;
        VkDeviceMemory depthCubeMemory;
        VkImageView cubeImageView;                    // For sampling in main shader
        std::array<VkImageView, CUBE_FACES> faceViews; // For rendering to each face
        VkSampler sampler;
        VkRenderPass renderPass;
        std::array<VkFramebuffer, CUBE_FACES> framebuffers; // One per face!

        // Helper functions
        void createDepthCubeImage();
        void createImageViews();
        void createSampler();
        void createRenderPass();
        void createFramebuffers();
    };

    class ShadowManager {
    public:

        ShadowManager(VeDevice& device);
        ~ShadowManager();

        ShadowManager(const ShadowManager &) = delete;
        ShadowManager& operator=(const ShadowManager &) = delete;
        ShadowManager(ShadowManager &&) = delete;
        ShadowManager& operator=(ShadowManager &&) = delete;

        // Initialize all shadow cube maps
        bool initialize();
        void cleanup();

        // Main function - renders shadow maps for all active lights
        void renderShadowMaps(VkCommandBuffer commandBuffer,
                              const std::vector<PointLight>& lights,
                              VkPipelineLayout shadowPipelineLayout,
                              VkPipeline shadowPipeline,
                              std::function<void(VkCommandBuffer, int, int, const glm::mat4&)> drawScene);

        // Get resources for main rendering pass
        VkImageView getShadowCubeViews(int index) const {return shadowCubeMaps[index]->getCubeImageView();};
        VkSampler getShadowSamplers(int index) const {return shadowCubeMaps[index]->getSampler();};
        VkImageLayout getLayout() const {return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;};
        VkRenderPass getShadowRenderPass(int lightIndex) const { return shadowCubeMaps[lightIndex]->getRenderPass(); };

    private:
        VeDevice& device;
        std::vector<std::unique_ptr<ShadowCubeMap>> shadowCubeMaps; // One per light
    };
}
#endif //VULKANANDROID_SHADOW_MANAGER_HPP
