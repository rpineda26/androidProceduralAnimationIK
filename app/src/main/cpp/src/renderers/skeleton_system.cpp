#include "skeleton_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <stdexcept>
#include <cassert>
#include <iostream>
namespace ve {
    struct JointPushConstantData {
        alignas(16) glm::mat4 modelMatrix{1.0f};
        alignas(16) float size{0.5f};
        alignas(16) glm::vec4 color{1.0f};
    };
    struct LinePushConstantData {
        glm::vec3 startPos;
        float pad1{1.0f};  // Padding for alignment
        glm::vec3 endPos;
        float pad2{1.0f};  // Padding for alignment
        glm::vec4 color{1.0f};
    };

    SkeletonSystem::SkeletonSystem(
        VeDevice& device, AAssetManager *assetManager,
        VkRenderPass renderPass,
        const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts
    ): veDevice{device} {
        if(assetManager==nullptr)
            throw std::runtime_error("SkeletonSystem::SkeletonSystem: assetManager is nullptr");
        createPipelineLayout(descriptorSetLayouts);
        createPipeline(assetManager, renderPass);
    }
    SkeletonSystem::~SkeletonSystem() {
        vkDestroyPipelineLayout(veDevice.device(), jointSpheresPipelineLayout, nullptr);
        vkDestroyPipelineLayout(veDevice.device(), boneLinesPipelineLayout, nullptr);
    }


    void SkeletonSystem::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
        //joints
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(JointPushConstantData);

   
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &jointSpheresPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create joints pipeline layout!");
        }
        //bones
        VkPushConstantRange pushConstantRange2{};
        pushConstantRange2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange2.offset = 0;
        pushConstantRange2.size = sizeof(LinePushConstantData);


        VkPipelineLayoutCreateInfo pipelineLayoutInfo2{};
        pipelineLayoutInfo2.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo2.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo2.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo2.pushConstantRangeCount = 1;
        pipelineLayoutInfo2.pPushConstantRanges = &pushConstantRange2;
        if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo2, nullptr, &boneLinesPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create bones pipeline layout!");
        }
    }
    void SkeletonSystem::createPipeline(AAssetManager *assetManager, VkRenderPass renderPass) {
        assert(jointSpheresPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        assert(boneLinesPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        //joint
        PipelineConfigInfo pipelineConfig{};
        VePipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.vertexAttributeDescriptions.clear();
        pipelineConfig.vertexBindingDescriptions.clear();
        pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = jointSpheresPipelineLayout;
        jointSpheresPipeline = std::make_unique<VePipeline>(
            veDevice,
            assetManager,
            "shaders/joints_shader.vert.spv",
            "shaders/joints_shader.frag.spv",
            pipelineConfig
        );
        //bone
        PipelineConfigInfo pipelineConfig2{};
        VePipeline::defaultPipelineConfigInfo(pipelineConfig2);
        pipelineConfig2.vertexAttributeDescriptions.clear();
        pipelineConfig2.vertexBindingDescriptions.clear();
        pipelineConfig2.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        pipelineConfig2.rasterizationInfo.lineWidth = 1.0f;  // Adjust thickness here
        pipelineConfig2.renderPass = renderPass;
        pipelineConfig2.pipelineLayout = boneLinesPipelineLayout;
        boneLinesPipeline = std::make_unique<VePipeline>(
                veDevice,
                assetManager,
                "shaders/bones_shader.vert.spv",
                "shaders/bones_shader.frag.spv",
                pipelineConfig2
        );
    }

    void SkeletonSystem::renderJoints(FrameInfo& frameInfo, const std::vector<VkDescriptorSet>& descriptorSets) {
        jointSpheresPipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            jointSpheresPipelineLayout,
            0,  // First set index
            static_cast<uint32_t>(descriptorSets.size()), // Number of sets
            descriptorSets.data(), // Pointer to descriptor sets
            0,
            nullptr
        );
        auto& gameObject = frameInfo.gameObjects.at(frameInfo.selectedObject);
        if (!gameObject.model->hasAnimationData()) return;

        glm::mat4 worldTransform = gameObject.transform.mat4();
        auto* skeleton = gameObject.model->skeleton.get();
        int numJoints = skeleton->joints.size();
        std::vector<glm::mat4> globalJointTransforms(numJoints);

        for (int i = 0; i < numJoints; i++) {
            if (skeleton->isAnimated) {
                // Get local transform matrix from TRS components
                globalJointTransforms[i] = skeleton->joints[i].getLocalMatrix();

                // Apply parent transforms if not root
                int16_t parentIndex = skeleton->joints[i].parentIndex;
                if (parentIndex != ve::NO_PARENT) {
                    globalJointTransforms[i] = globalJointTransforms[parentIndex] * globalJointTransforms[i];
                }
            } else {
                // Use joint's bind pose position
                globalJointTransforms[i] = skeleton->joints[i].jointWorldMatrix;
            }
        }

        // Now render each joint
        for (int i = 0; i < numJoints; i++) {
            // Extract the joint position from the translation component of the global transform
            glm::vec3 jointPos = glm::vec3(globalJointTransforms[i][3]);
            if (glm::length(jointPos) < 0.001f) {
                continue;
            }
            // Transform to world space
            glm::vec4 jointPosWorldSpace = worldTransform * glm::vec4(jointPos, 1.0f);

            // Create a transform matrix for the joint sphere
            glm::mat4 pointMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(jointPosWorldSpace));
            float jointSize = 0.03f;
            pointMatrix = glm::scale(pointMatrix, glm::vec3(jointSize));

            JointPushConstantData push{};
            push.modelMatrix = pointMatrix;
            if (frameInfo.selectedJointIndex!=-1 && i == frameInfo.selectedJointIndex) {
                push.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // Highlight color (yellow)
            } else {
                push.color = glm::vec4(1.0f); // Regular color (light blue)
            }
            vkCmdPushConstants(
                    frameInfo.commandBuffer,
                    jointSpheresPipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(JointPushConstantData),
                    &push
            );

            vkCmdDraw(frameInfo.commandBuffer, 1, 1, 0, 0);
        }
    }
    void SkeletonSystem::renderJointConnections(FrameInfo& frameInfo, const std::vector<VkDescriptorSet>& descriptorSets) {
        boneLinesPipeline->bind(frameInfo.commandBuffer);
        vkCmdBindDescriptorSets(
                frameInfo.commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                boneLinesPipelineLayout,
                0,
                static_cast<uint32_t>(descriptorSets.size()),
                descriptorSets.data(),
                0,
                nullptr
        );

        auto& gameObject = frameInfo.gameObjects.at(frameInfo.selectedObject);
        if (!gameObject.model->hasAnimationData()) return;

        glm::mat4 worldTransform = gameObject.transform.mat4();
        auto* skeleton = gameObject.model->skeleton.get();
        int numJoints = skeleton->joints.size();

        // Save current joint matrices
        std::vector<glm::mat4> savedMatrices = skeleton->jointMatrices;

        // Calculate joint positions without applying inverse bind matrices
        std::vector<glm::vec3> jointPositions(numJoints);
        std::vector<bool> validJoint(numJoints, false);

        if (skeleton->isAnimated) {
            // Apply animation transforms
            for (int16_t i = 0; i < numJoints; i++) {
                skeleton->jointMatrices[i] = skeleton->joints[i].getAnimatedMatrix();
            }

            // Now calculate the global transforms manually
            for (int i = 0; i < numJoints; i++) {
                glm::mat4 globalMatrix = skeleton->jointMatrices[i];
                int parentIndex = skeleton->joints[i].parentIndex;

                if (parentIndex != ve::NO_PARENT) {
                    // Apply parent transform (since we can't use updateJoint)
                    globalMatrix = skeleton->jointMatrices[parentIndex] * globalMatrix;

                    // Update the matrix for this joint
                    skeleton->jointMatrices[i] = globalMatrix;
                }

                // Store the joint position
                jointPositions[i] = glm::vec3(globalMatrix[3]);

                // Mark as valid if not at origin
                validJoint[i] = (glm::length(jointPositions[i]) >= 0.001f);
            }

            // Render lines connecting joints based on parent-child relationships
            for (int i = 0; i < numJoints; i++) {
                int parentIndex = skeleton->joints[i].parentIndex;

                // Skip if there's no parent or either joint is invalid
                if (parentIndex == ve::NO_PARENT || !validJoint[i] || !validJoint[parentIndex]) {
                    continue;
                }

                // Get positions in world space
                glm::vec3 childPosWorldSpace = glm::vec3(worldTransform * glm::vec4(jointPositions[i], 1.0f));
                glm::vec3 parentPosWorldSpace = glm::vec3(worldTransform * glm::vec4(jointPositions[parentIndex], 1.0f));

                LinePushConstantData push{};

                push.startPos = parentPosWorldSpace;
                push.endPos = childPosWorldSpace;
                if((i-1==frameInfo.selectedJointIndex)|| (i ==frameInfo.selectedJointIndex))
                    push.color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // Highlight color
                else
                    push.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

                vkCmdPushConstants(
                        frameInfo.commandBuffer,
                        boneLinesPipelineLayout,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,
                        sizeof(LinePushConstantData),
                        &push
                );

                // Draw a line with 2 vertices
                vkCmdDraw(frameInfo.commandBuffer, 2, 1, 0, 0);
            }
        }

        // Restore original matrices
        skeleton->jointMatrices = savedMatrices;
    }
}