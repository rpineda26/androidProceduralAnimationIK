#pragma once

#include "ve_camera.hpp"
#include "ve_game_object.hpp"
#include <vulkan/vulkan.h>

#define MAX_POINT_LIGHTS 10
namespace ve{
    struct PointLightShadowMap{
        glm::mat4 lightSpaceMatrix[6];
        glm::vec4 lightPosition;
    };
    struct LightShadowUbo{
        PointLightShadowMap pointLightShadowMap;
        int numLights;
    };
    struct alignas(16)PointLight{
        glm::vec4 position{}; //ignore w, align with 16 bytes
        glm::vec4 color{}; // w is intensity
        float radius;
        int objId;
        // Color{1.0f, 0.9f, 0.5f, 1.0f}; yellowish
        // Color{213.0f,185.0f,255.0f,1.0f}; purple
    };
    struct alignas(16) GlobalUbo {
        glm::mat4 projection{1.0f};
        glm::mat4 view{1.0f};
        glm::mat4 inverseView{1.0f};
        glm::vec4 ambientLightColor{1.0f,1.0f,1.0f,0.07f};
        PointLight pointLights[MAX_POINT_LIGHTS];
        int numLights;
        alignas(16) int selectedLight;
        alignas(16)float frameTime;
    };

    struct FrameInfo{
        int frameIndex;
        float frameTime;
        float elapsedTime;
        VkCommandBuffer commandBuffer;
        VeCamera& camera;
        VkDescriptorSet descriptorSet; // camera, lights (global ubo)
        VkDescriptorSet animatedDescriptorSet;
        VeGameObject::Map& gameObjects;
        int selectedObject;
        int selectedJointIndex;
        int numLights;
        bool showOutlignHighlight;
    };
    struct EngineInfo{
        float frameTime{0.0f};
        float elapsedTime{0.0f};
        std::chrono::time_point<std::chrono::steady_clock,std::chrono::duration<long long, std::ratio<1,1000000000>>>
                currentTime = std::chrono::high_resolution_clock::now();;
        int numLights{0};
        int frameCount{0};
        int frameIndex{0};
        int cubeMapIndex{3};
        int animatedObjIndex{1};
        int selectedObject{1};
        int selectedJointIndex{-1};
        int currentAnimationIndex{5};
        bool showOutlignHighlight{true};
        bool engineInitialized{false};
        bool showModelsList{false};
        bool showEditJointMode{false};
        bool changeAnimationList{false};
        std::string currentLoadingModelName = "";
        VeCamera camera{};
        VeGameObject viewerObject = VeGameObject::createGameObject();
    };
}