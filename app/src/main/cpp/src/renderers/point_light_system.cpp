#include "point_light_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
#include <map>
namespace ve {
    PointLightSystem::PointLightSystem(
        VeDevice& device, AAssetManager *assetManager,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout
    ): veDevice{device} {
        createPipelineLayout(globalSetLayout);
        createPipeline(assetManager, renderPass);
    }
    PointLightSystem::~PointLightSystem() {
        vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
    }


    void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        // VkPushConstantRange pushConstantRange{};
        // pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        // pushConstantRange.offset = 0;
        // pushConstantRange.size = sizeof(SimplePushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
    void PointLightSystem::createPipeline(AAssetManager *assetManager, VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        PipelineConfigInfo pipelineConfig{};
        VePipeline::defaultPipelineConfigInfo(pipelineConfig);
        VePipeline::enableAlphaBlending(pipelineConfig); 
        pipelineConfig.vertexAttributeDescriptions.clear();
        pipelineConfig.vertexBindingDescriptions.clear();
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        vePipeline = std::make_unique<VePipeline>(
            veDevice,
            assetManager,
            "shaders/point_light_shader.vert.spv",
            "shaders/point_light_shader.frag.spv",
            pipelineConfig);
    }
    
    void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo) {
        int lightIndex = 0;
        for(auto& key_value : frameInfo.gameObjects){
            auto& object = key_value.second;
            //working with point lights
            if(object.lightComponent!=nullptr) {
                ubo.pointLights[lightIndex].position = glm::vec4(object.transform.translation,1.0f); //vec3 position is aligned as vec4
                ubo.pointLights[lightIndex].color = glm::vec4(object.color, object.lightComponent->lightIntensity);
                ubo.pointLights[lightIndex].radius = object.transform.scale.x;
                ubo.pointLights[lightIndex].objId = object.getId();
                
                //pulsate
                if(ubo.pointLights[lightIndex].objId == frameInfo.selectedObject && frameInfo.showOutlignHighlight){
                    float pulseRadius = 1.0f + 0.2f * glm::sin(frameInfo.elapsedTime * 5.0f); 
                    float pulseColor = 0.5f + 1.0f * glm::sin(frameInfo.elapsedTime * 5.0f); 
                    ubo.pointLights[lightIndex].color *= pulseColor;
                    ubo.pointLights[lightIndex].radius *= pulseRadius;
                }
                lightIndex++;
            }
        }
        ubo.numLights = lightIndex;
    }
    void PointLightSystem::render(FrameInfo& frameInfo) {

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
        vkCmdDraw(frameInfo.commandBuffer, 6, frameInfo.numLights, 0, 0);
    }
}