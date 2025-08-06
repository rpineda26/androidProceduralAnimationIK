#pragma once
#include "ve_device.hpp"

#include <android/asset_manager.h>

#include <string>
#include <vector>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace ve {
    struct PipelineConfigInfo {
        PipelineConfigInfo() = default;
        PipelineConfigInfo(const PipelineConfigInfo&) = delete;
        PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
        
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    }; 
    class VePipeline {
        public:

            VePipeline(VeDevice& device, AAssetManager *assetManager, const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo);
            ~VePipeline();
            VePipeline(const VePipeline&) = delete;
            VePipeline& operator=(const VePipeline&) = delete;
            void bind(VkCommandBuffer commandBuffer);
            static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
            static void enableAlphaBlending(PipelineConfigInfo& configInfo);
            void createShaderModule(const std::vector<uint8_t>& code, VkShaderModule* shaderModule);

            VkPipeline getPipeline() { return graphicsPipeline; }
        private:
            void createGraphicsPipeline(AAssetManager *assetManager, const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo);
            

            VeDevice& veDevice;
            VkPipeline graphicsPipeline;
            VkShaderModule vertShaderModule;
            VkShaderModule fragShaderModule;
    };
}