#pragma once
#include "ve_game_object.hpp"
#include "ve_device.hpp"
#include "ve_window.hpp"
#include <imgui.h>
#include <imgui_impl_android.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <game-activity/native_app_glue/android_native_app_glue.h>

namespace ve{
    enum class Orientation{
        PORTRAIT,
        LANDSCAPE_LEFT,
        LANDSCAPE_RIGHT,
        PORTRAIT_REVERSED
    };
    class VeImGui{
        public:
        static VkDescriptorPool createDescriptorPool(VkDevice device);
        static VkRenderPass createRenderPass(VkDevice device, VkFormat imageFormat, VkFormat depthFormat);
        static void createImGuiContext( VeDevice& veDevice, VeWindow& veWindow, VkDescriptorPool imGuiPool, VkRenderPass renderPass, int imageCount);
        static void initializeImGuiFrame();
        static void renderImGuiFrame(VkCommandBuffer commandBuffer);
        static bool handleInput(const GameActivityMotionEvent* event);
        static void updateImGuiTransform(uint32_t displayWidth, uint32_t displayHeight, Orientation currentOrientation);
        static void cleanUpImGui();
    };
}