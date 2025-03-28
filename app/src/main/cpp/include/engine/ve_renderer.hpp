#pragma once
#include "ve_window.hpp"
#include "ve_device.hpp"
#include "ve_swap_chain.hpp"
#include <memory>
#include <vector>
#include <cassert>
namespace ve {
    class VeRenderer{
        public:

            VeRenderer(VeWindow& window, VeDevice& device);
            ~VeRenderer();
            VeRenderer(const VeRenderer&) = delete;
            VeRenderer& operator=(const VeRenderer&) = delete;

            //methods
            VkCommandBuffer beginFrame();
            void endFrame();
            
            void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
            void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

            //getters
            VkRenderPass getSwapChainRenderPass() const { return veSwapChain->getRenderPass(); }
            bool isFrameInProgress() const { return isFrameStarted; }
            VkCommandBuffer getCurrentCommandBuffer() const { 
                assert(isFrameStarted && "Cannot get command buffer when frame not in progress.");
                return commandBuffers[currentFrameIndex]; 
            }
            int getFrameIndex() const { 
                assert(isFrameStarted && "Cannot get frame index when frame not in progress.");
                return currentFrameIndex; 
            }
            float getAspectRatio() const { return veSwapChain->extentAspectRatio(); }
            VkFormat getSwapChainImageFormat() const { return veSwapChain->getSwapChainImageFormat(); }
            VkFormat getSwapChainDepthFormat() const { return veSwapChain->findDepthFormat(); }
            void resetWindow();
            VkSurfaceTransformFlagBitsKHR getPreRotTransformFlag() const { return veSwapChain->getPreTransformFlag(); }
        private:
            void createCommandBuffers();
            void freeCommandBuffers();
            void recreateSwapChain();

            VeWindow& veWindow;
            VeDevice& veDevice;
            std::unique_ptr<VeSwapChain> veSwapChain;
            std::vector<VkCommandBuffer> commandBuffers;

            uint32_t currentImageIndex{0};
            int currentFrameIndex{0};
            bool isFrameStarted{false};
    };
}