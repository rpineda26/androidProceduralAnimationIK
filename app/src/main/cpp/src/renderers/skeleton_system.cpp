#include "skeleton_system.hpp"
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
    struct JointPushConstantData {
        alignas(16) glm::mat4 modelMatrix{1.0f};
        alignas(16) float size{0.5f};
        alignas(16) glm::vec4 color{1.0f};
    };
    struct LinePushConstantData {
        alignas(16) glm::mat4 modelMatrix{1.0f};
        alignas(16) glm::vec4 color{1.0f};
    };

    SkeletonSystem::SkeletonSystem(
        VeDevice& device, AAssetManager *assetManager,
        VkRenderPass renderPass,
        const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
        float aspect
    ): veDevice{device} {
        if(assetManager==nullptr)
            throw std::runtime_error("SkeletonSystem::SkeletonSystem: assetManager is nullptr");
        aspectRatio = aspect;
        createPipelineLayout(descriptorSetLayouts);
        createPipeline(assetManager, renderPass);
        initializeConeGeometry();
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
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
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

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;        // Cull back faces
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;     // Clockwise = front face
        rasterizer.depthBiasEnable = VK_TRUE;

        pipelineConfig2.rasterizationInfo = rasterizer;
        pipelineConfig2.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        pipelineConfig2.vertexBindingDescriptions = ConeMesh::Vertex::getBindingDescriptions();
        pipelineConfig2.vertexAttributeDescriptions =  ConeMesh::Vertex::getAttributeDescriptions();
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
    void SkeletonSystem::initializeConeGeometry() {
        coneMesh = std::make_unique<ConeMesh>();
        coneMesh->generateCone(8); // 8-sided cone

        // Create vertex buffer
        VkDeviceSize vertexBufferSize = sizeof(ConeMesh::Vertex) * coneMesh->vertices.size();
        coneVertexBuffer = std::make_unique<VeBuffer>(
                veDevice,
                sizeof(ConeMesh::Vertex),
                coneMesh->vertices.size(),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        coneVertexBuffer->map();
        coneVertexBuffer->writeToBuffer(coneMesh->vertices.data());

        // Create index buffer
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * coneMesh->indices.size();
        coneIndexBuffer = std::make_unique<VeBuffer>(
                veDevice,
                sizeof(uint32_t),
                coneMesh->indices.size(),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        coneIndexBuffer->map();
        coneIndexBuffer->writeToBuffer(coneMesh->indices.data());
    }
    glm::mat4 SkeletonSystem::createConeTransform(const glm::vec3& start, const glm::vec3& end) {
        glm::vec3 direction = end - start;
        float length = glm::length(direction);

        if (length < 0.001f) return glm::mat4(1.0f); // Degenerate case

        direction = normalize(direction);

        // Create rotation matrix to align cone with bone direction
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        // Handle case where direction is parallel to up vector
        if (abs(dot(direction, up)) > 0.99f) {
            up = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        glm::vec3 right = normalize(cross(direction, up));
        up = cross(right, direction);

        glm::mat4 rotation = glm::mat4(
                glm::vec4(right, 0.0f),
                glm::vec4(direction, 0.0f), // Y axis points along bone
                glm::vec4(up, 0.0f),
                glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
        );

        // Create transform: translate to start, rotate to align, scale by length
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), start) *
                              rotation *
                              glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, length, 1.0f));

        return transform;
    }
    void SkeletonSystem::renderJoints(FrameInfo& frameInfo, const std::vector<VkDescriptorSet>& descriptorSets) {
        auto& gameObject = frameInfo.gameObjects.at(frameInfo.selectedObject);
        if (!gameObject.model->hasAnimationData()) return;
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
                push.color = glm::vec4(0.0f, 0.9f, 1.0f, 1.0f);
            } else {
                push.color = glm::vec4(1.0f, 1.0f, 1.0f, aspectRatio); // Regular color (light blue)
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
        auto& gameObject = frameInfo.gameObjects.at(frameInfo.selectedObject);
        if (!gameObject.model->hasAnimationData()) return;

        // Bind the cone pipeline (instead of line pipeline)
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

        // Bind cone geometry
        VkBuffer vertexBuffers[] = {coneVertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(frameInfo.commandBuffer, coneIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

        glm::mat4 worldTransform = gameObject.transform.mat4();
        auto* skeleton = gameObject.model->skeleton.get();
        int numJoints = skeleton->joints.size();

        // Same joint position calculation as before...
        std::vector<glm::mat4> savedMatrices = skeleton->jointMatrices;
        std::vector<glm::vec3> jointPositions(numJoints);
        std::vector<bool> validJoint(numJoints, false);

        if (skeleton->isAnimated) {
            // [Same animation calculation code as your original...]
            for (int16_t i = 0; i < numJoints; i++) {
                skeleton->jointMatrices[i] = skeleton->joints[i].getAnimatedMatrix();
            }

            for (int i = 0; i < numJoints; i++) {
                glm::mat4 globalMatrix = skeleton->jointMatrices[i];
                int parentIndex = skeleton->joints[i].parentIndex;

                if (parentIndex != ve::NO_PARENT) {
                    globalMatrix = skeleton->jointMatrices[parentIndex] * globalMatrix;
                    skeleton->jointMatrices[i] = globalMatrix;
                }

                jointPositions[i] = glm::vec3(globalMatrix[3]);
                validJoint[i] = (glm::length(jointPositions[i]) >= 0.001f);
            }

            // Render cones instead of lines
            for (int i = 0; i < numJoints; i++) {
                int parentIndex = skeleton->joints[i].parentIndex;

                if (parentIndex == ve::NO_PARENT || !validJoint[i] || !validJoint[parentIndex]) {
                    continue;
                }

                glm::vec3 childPosWorldSpace = glm::vec3(worldTransform * glm::vec4(jointPositions[i], 1.0f));
                glm::vec3 parentPosWorldSpace = glm::vec3(worldTransform * glm::vec4(jointPositions[parentIndex], 1.0f));

                // Create cone transformation matrix
                glm::mat4 coneModelMatrix = createConeTransform(parentPosWorldSpace, childPosWorldSpace);
                LinePushConstantData push{};
                push.modelMatrix = coneModelMatrix;

                if (i == frameInfo.selectedJointIndex) {
                    push.color = glm::vec4(0.0f, 0.9f, 1.0f, 1.0f);  // Light blue highlight, more opaque
                } else {
                    push.color = glm::vec4(0.3f, 0.35f, 0.2f, 0.7f);
                }

                vkCmdPushConstants(
                        frameInfo.commandBuffer,
                        boneLinesPipelineLayout,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,
                        sizeof(LinePushConstantData),
                        &push
                );

                // Draw the cone using indexed rendering
                vkCmdDrawIndexed(frameInfo.commandBuffer,
                                 static_cast<uint32_t>(coneMesh->indices.size()),
                                 1, 0, 0, 0);
            }
        }

        skeleton->jointMatrices = savedMatrices;
    }
}