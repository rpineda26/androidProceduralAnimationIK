//
// Created by Ralph Dawson Pineda on 7/3/25.
//
#include "shadow_manager.hpp"
#include <stdexcept>
namespace ve{
    // Implementation of ShadowCubeMap
    ShadowCubeMap::ShadowCubeMap(VeDevice& veDevice) : device(veDevice) {
        // Initialize all handles to null
        depthCubeImage = VK_NULL_HANDLE;
        depthCubeMemory = VK_NULL_HANDLE;
        cubeImageView = VK_NULL_HANDLE;
        sampler = VK_NULL_HANDLE;
        renderPass = VK_NULL_HANDLE;

        for (int i = 0; i < CUBE_FACES; i++) {
            faceViews[i] = VK_NULL_HANDLE;
            framebuffers[i] = VK_NULL_HANDLE;
        }
    }

    ShadowCubeMap::~ShadowCubeMap() {
        cleanup();
    }

    bool ShadowCubeMap::create() {
        try {
            createDepthCubeImage();
            createImageViews();
            createSampler();
            createRenderPass();
            createFramebuffers();
            return true;
        } catch (const std::exception& e) {
            cleanup();
            return false;
        }
    }

    void ShadowCubeMap::cleanup() {
        // Clean up in reverse order
        for (int i = 0; i < CUBE_FACES; i++) {
            if (framebuffers[i] != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(device.device(), framebuffers[i], nullptr);
                framebuffers[i] = VK_NULL_HANDLE;
            }
        }

        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device.device(), renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }

        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device.device(), sampler, nullptr);
            sampler = VK_NULL_HANDLE;
        }

        for (int i = 0; i < CUBE_FACES; i++) {
            if (faceViews[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(device.device(), faceViews[i], nullptr);
                faceViews[i] = VK_NULL_HANDLE;
            }
        }

        if (cubeImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device.device(), cubeImageView, nullptr);
            cubeImageView = VK_NULL_HANDLE;
        }

        if (depthCubeImage != VK_NULL_HANDLE) {
            vkDestroyImage(device.device(), depthCubeImage, nullptr);
            depthCubeImage = VK_NULL_HANDLE;
        }

        if (depthCubeMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device.device(), depthCubeMemory, nullptr);
            depthCubeMemory = VK_NULL_HANDLE;
        }
    }

// THIS IS THE KEY FUNCTION - renders to all 6 faces of the cube
    void ShadowCubeMap::recordShadowPass(VkCommandBuffer commandBuffer,
                                         const glm::vec3& lightPos,
                                         VkPipelineLayout shadowPipelineLayout,
                                         VkPipeline shadowPipeline,
                                         std::function<void(VkCommandBuffer, int, const glm::mat4&)> drawScene) {

        auto viewMatrices = getViewMatrices(lightPos);
        auto projMatrix = getProjectionMatrix();

        for (int face = 0; face < CUBE_FACES; face++) {
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = framebuffers[face];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};

            // Only one clear value for depth attachment
            VkClearValue clearValue{};
            clearValue.depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = 1;  // Only 1 clear value
            renderPassInfo.pClearValues = &clearValue;

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(SHADOW_MAP_SIZE);
            viewport.height = static_cast<float>(SHADOW_MAP_SIZE);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{{0, 0}, {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            glm::mat4 viewProjMatrix = projMatrix * viewMatrices[face];
            drawScene(commandBuffer, face, viewProjMatrix);

            vkCmdEndRenderPass(commandBuffer);
        }
    }

    std::array<glm::mat4, 6> ShadowCubeMap::getViewMatrices(const glm::vec3& lightPos) const {
        std::array<glm::mat4, 6> viewMatrices;

        // Standard cube map face orientations
        viewMatrices[0] = glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)); // +X
        viewMatrices[1] = glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)); // -X
        viewMatrices[2] = glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)); // +Y
        viewMatrices[3] = glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)); // -Y
        viewMatrices[4] = glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)); // +Z
        viewMatrices[5] = glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)); // -Z

        return viewMatrices;
    }

    glm::mat4 ShadowCubeMap::getProjectionMatrix() const {
        // 90-degree FOV perspective projection for cube faces
        return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 30.0f);
    }

    void ShadowCubeMap::createDepthCubeImage() {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = SHADOW_MAP_SIZE;
        imageInfo.extent.height = SHADOW_MAP_SIZE;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = CUBE_FACES;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;  // Back to depth format
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;  // Back to depth
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        if (vkCreateImage(device.device(), &imageInfo, nullptr, &depthCubeImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow cube map image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device.device(), depthCubeImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device.device(), &allocInfo, nullptr, &depthCubeMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate shadow cube map memory!");
        }

        vkBindImageMemory(device.device(), depthCubeImage, depthCubeMemory, 0);
    }

    void ShadowCubeMap::createImageViews() {
        // Create main cube map view for sampling
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthCubeImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;  // Back to depth format
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;  // Back to depth aspect
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = CUBE_FACES;

        if (vkCreateImageView(device.device(), &viewInfo, nullptr, &cubeImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow cube map image view!");
        }

        // Create individual face views for rendering
        for (int i = 0; i < CUBE_FACES; i++) {
            VkImageViewCreateInfo faceViewInfo{};
            faceViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            faceViewInfo.image = depthCubeImage;
            faceViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            faceViewInfo.format = VK_FORMAT_D32_SFLOAT;  // Back to depth format
            faceViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;  // Back to depth aspect
            faceViewInfo.subresourceRange.baseMipLevel = 0;
            faceViewInfo.subresourceRange.levelCount = 1;
            faceViewInfo.subresourceRange.baseArrayLayer = i;
            faceViewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device.device(), &faceViewInfo, nullptr, &faceViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create shadow cube map face view!");
            }
        }
    }

    void ShadowCubeMap::createSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_TRUE;  // Enable depth comparison
        samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow cube map sampler!");
        }
    }

    void ShadowCubeMap::createRenderPass() {
        // Use only depth attachment - simpler and more appropriate for shadow mapping
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;  // No color attachments
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow map render pass!");
        }
    }

    void ShadowCubeMap::createFramebuffers() {
        // This is problematic - you're creating 6 framebuffers but all using the same cube view
        // You need to use individual face views for each framebuffer

        for(int i = 0; i < CUBE_FACES; i++) {
            // Use the individual face view for each framebuffer
            VkImageView attachments[] = { faceViews[i] };  // Use face view, not cube view

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;  // Will need to be 2 if you add depth buffer
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = SHADOW_MAP_SIZE;
            framebufferInfo.height = SHADOW_MAP_SIZE;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create shadow map framebuffer!");
            }
        }
    }

    // Implementation of ShadowManager
    ShadowManager::ShadowManager(VeDevice& veDevice) : device(veDevice) {
        shadowCubeMaps.reserve(MAX_POINT_LIGHTS);
    }

    ShadowManager::~ShadowManager() {
        cleanup();
    }

    bool ShadowManager::initialize() {
        try {
            // Create one shadow cube map for each potential light
            for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
                auto shadowMap = std::make_unique<ShadowCubeMap>(device);
                if (!shadowMap->create()) {
                    throw std::runtime_error("Failed to create shadow cube map " + std::to_string(i));
                }
                shadowCubeMaps.push_back(std::move(shadowMap));
            }
            return true;
        } catch (const std::exception& e) {
            cleanup();
            return false;
        }
    }

    void ShadowManager::cleanup() {
        shadowCubeMaps.clear(); // Automatically calls destructors
    }

// THIS IS THE MAIN FUNCTION - coordinates rendering for all lights
    void ShadowManager::renderShadowMaps(VkCommandBuffer commandBuffer,
                                         const std::vector<PointLight>& lights,
                                         VkPipelineLayout shadowPipelineLayout,
                                         VkPipeline shadowPipeline,
                                         std::function<void(VkCommandBuffer, int, int, const glm::mat4&)> drawScene) {

        int lightIndex = 0;
        for (const auto& light : lights) {
            if (lightIndex >= MAX_POINT_LIGHTS) break;

            // Render shadow cube map for this specific light
            shadowCubeMaps[lightIndex]->recordShadowPass(
                    commandBuffer,
                    light.position,
                    shadowPipelineLayout,
                    shadowPipeline,
                    [&](VkCommandBuffer cmd, int faceIndex, const glm::mat4& viewProjMatrix) {
                        // Call the main draw function with light index, face index, and matrices
                        drawScene(cmd, lightIndex, faceIndex, viewProjMatrix);
                    }
            );

            lightIndex++;
        }

        // Memory barrier to ensure shadow maps are ready for sampling
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 1, &barrier, 0, nullptr, 0, nullptr);
    }


}