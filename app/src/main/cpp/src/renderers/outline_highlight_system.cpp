#include "outline_highlight_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {
    struct SimplePushConstantData {
        glm::mat4 modelMatrix{1.0f};
    };

    OutlineHighlightSystem::OutlineHighlightSystem(
        VeDevice& device, AAssetManager *assetManager,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout
    ): veDevice{device} {
        createPipelineLayout(globalSetLayout);
        createPipeline(assetManager, renderPass);
    }
    OutlineHighlightSystem::~OutlineHighlightSystem() {
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
    }


    void OutlineHighlightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

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
    void OutlineHighlightSystem::createPipeline(AAssetManager *assetManager, VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        PipelineConfigInfo pipelineConfig{};
        VePipeline::defaultPipelineConfigInfo(pipelineConfig);
        
        //set rasterization state to enable culling for outline
        VkPipelineRasterizationStateCreateInfo rasterizationState{};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
        rasterizationState.depthBiasEnable = VK_FALSE;
        rasterizationState.depthClampEnable = VK_FALSE;
        rasterizationState.rasterizerDiscardEnable = VK_FALSE;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.lineWidth = 1.0f;

        pipelineConfig.rasterizationInfo = rasterizationState;
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        vePipeline = std::make_unique<VePipeline>(
            veDevice,
            assetManager,
            "shaders/outline_highlight_shader.vert.spv",
            "shaders/outline_highlight_shader.frag.spv",
            pipelineConfig);
    }
    
    
    void OutlineHighlightSystem::renderGameObjects(FrameInfo& frameInfo) {
        vePipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &frameInfo.descriptorSet,
            0,
            nullptr
        );
        for(auto& key_value : frameInfo.gameObjects){
            auto& obj = key_value.second;
            if(obj.getId() == frameInfo.selectedObject && obj.lightComponent == nullptr){
                SimplePushConstantData push{};
                push.modelMatrix =  obj.transform.mat4();
                
                vkCmdPushConstants(
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(SimplePushConstantData),
                    &push
                );
                obj.model->bind(frameInfo.commandBuffer);
                obj.model->draw(frameInfo.commandBuffer);
            }
        }
    }
}