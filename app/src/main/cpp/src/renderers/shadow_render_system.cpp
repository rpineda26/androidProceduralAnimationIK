//
// Created by Ralph Dawson Pineda on 7/3/25.
//
#include "shadow_render_system.hpp"
namespace ve{
    ShadowRenderSystem::ShadowRenderSystem(VeDevice& device, AAssetManager* assetManager,
                                           VkRenderPass shadowRenderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
            : veDevice{device} {
        createPipelineLayout(descriptorSetLayouts);
        createPipeline(assetManager, shadowRenderPass);
    }

    ShadowRenderSystem::~ShadowRenderSystem() {
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
    }

    void ShadowRenderSystem::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ShadowPushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow pipeline layout!");
        }
    }

    void ShadowRenderSystem::createPipeline(AAssetManager* assetManager, VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        VePipeline::defaultPipelineConfigInfo(pipelineConfig);

        // Shadow-specific pipeline configuration
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        // Disable color writes for shadow pass (depth only)
        pipelineConfig.colorBlendInfo.attachmentCount = 0;
        pipelineConfig.colorBlendInfo.pAttachments = nullptr;

        // Enable depth testing and writing
        pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;

        // Cull front faces to reduce shadow acne
        pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
        pipelineConfig.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        vePipeline = std::make_unique<VePipeline>(
                veDevice,
                assetManager,
                "shaders/shadow_depth.vert.spv",
                "shaders/shadow_depth.frag.spv",
                pipelineConfig
        );
    }

    void ShadowRenderSystem::renderObjectsForShadowFace(VkCommandBuffer commandBuffer,
                                                        const FrameInfo& frameInfo,
                                                        int lightIndex,
                                                        int faceIndex,
                                                        const glm::mat4& viewProjMatrix,
                                                        const glm::vec3& lightPos,
                                                        float lightRadius) {
        // The pipeline binding is already done by ShadowManager

        // Bind global descriptor set if needed
        if (frameInfo.animatedDescriptorSet != VK_NULL_HANDLE ) {
            vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    0, 1,
                    &frameInfo.animatedDescriptorSet,
                    0, nullptr
            );
        }

        // Render all shadow-casting objects for this face
        for (auto& [id, obj] : frameInfo.gameObjects) {

            // Skip lights and non-shadow casting objects
            if (obj.lightComponent != nullptr) continue;

            ShadowPushConstants push{};
            push.mvpMatrix = viewProjMatrix * obj.transform.mat4();
            push.lightPos = lightPos;
            if(obj.model->hasAnimationData())
                push.isAnimated=true;
            else
                push.isAnimated=false;

            vkCmdPushConstants(
                    commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(ShadowPushConstants),
                    &push
            );

            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }
    }
}
