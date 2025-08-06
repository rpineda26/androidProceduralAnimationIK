#include "pbr_render_system.hpp"
#include "debug.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
#include <iostream>
namespace ve {
    struct PbrPushConstantData {
        glm::mat4 modelMatrix{1.0f};
        uint32_t textureIndex{0};
        float smoothness{0.0f};
//        glm::vec3 baseColor{1.0f};
        bool isAnimated{false};
    };

    PbrRenderSystem::PbrRenderSystem(
        VeDevice& device, AAssetManager *assetManager,
        VkRenderPass renderPass,
        const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts
    ): veDevice{device} {
        if(assetManager==nullptr)
            throw std::runtime_error("PbrRenderSystem::PbrRenderSystem: assetManager is nullptr");
        createPipelineLayout(descriptorSetLayouts);
        createPipeline(assetManager, renderPass);
    }
    PbrRenderSystem::~PbrRenderSystem() {
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
    }


    void PbrRenderSystem::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PbrPushConstantData);

   
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
    void PbrRenderSystem::createPipeline(AAssetManager *assetManager, VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        PipelineConfigInfo pipelineConfig{};
        VePipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        vePipeline = std::make_unique<VePipeline>(
            veDevice,
            assetManager,
            "shaders/pbr_shader.vert.spv",
            "shaders/pbr_shader.frag.spv",
            pipelineConfig);
    }
    
    
    void PbrRenderSystem::renderGameObjects(FrameInfo& frameInfo, const std::vector<VkDescriptorSet>& descriptorSets) {
        vePipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,  // First set index
            static_cast<uint32_t>(descriptorSets.size()), // Number of sets
            descriptorSets.data(), // Pointer to descriptor sets
            0,
            nullptr
        );
        for(auto& key_value : frameInfo.gameObjects){
            auto& obj = key_value.second;
            if(obj.lightComponent == nullptr && obj.cubeMapComponent == nullptr){
                PbrPushConstantData push{};
                push.modelMatrix =  obj.transform.mat4();
                push.textureIndex = obj.getTextureIndex();
                push.smoothness = obj.getSmoothness();
                if(obj.model->hasAnimationData()){
                    push.isAnimated = true;
                    
                }else{
//                    LOGI("Object is not animated, object: %s", obj.getTitle());
                    push.isAnimated = false;
                }
                vkCmdPushConstants(
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(PbrPushConstantData),
                    &push
                );
                obj.model->bind(frameInfo.commandBuffer);
                obj.model->draw(frameInfo.commandBuffer);
            }
        }
    }
}