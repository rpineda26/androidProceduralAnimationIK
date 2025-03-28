#include "cube_map_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {
    struct CubeMapPushConstantData {
        glm::mat4 modelMatrix{1.0f};
    };

    CubeMapRenderSystem::CubeMapRenderSystem(
        VeDevice& device, AAssetManager *assetManager, VkRenderPass renderPass,
        const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts
    ): veDevice{device} {
        createPipelineLayout(descriptorSetLayouts);
        createPipeline(assetManager, renderPass);
    }
    CubeMapRenderSystem::~CubeMapRenderSystem() {
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
    }


    void CubeMapRenderSystem::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(CubeMapPushConstantData);

   
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
    void CubeMapRenderSystem::createPipeline(AAssetManager *assetManager, VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        PipelineConfigInfo pipelineConfig{};
        VePipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        pipelineConfig.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        vePipeline = std::make_unique<VePipeline>(
            veDevice,
            assetManager,
            "shaders/cube_map.vert.spv",
            "shaders/cube_map.frag.spv",
            pipelineConfig);
    }
    
    
    void CubeMapRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
        vePipeline->bind(frameInfo.commandBuffer);

        for(auto& key_value : frameInfo.gameObjects){
            auto& obj = key_value.second;
            if(obj.cubeMapComponent!= nullptr){
                CubeMapPushConstantData push{};
                push.modelMatrix =  obj.transform.mat4();

                vkCmdPushConstants(
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(CubeMapPushConstantData),
                    &push
                );
                std::vector<VkDescriptorSet> descriptorSets = {frameInfo.descriptorSet, obj.cubeMapComponent->descriptorSet};
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
                obj.model->bind(frameInfo.commandBuffer);
                obj.model->draw(frameInfo.commandBuffer);
            }
        }
    }
}