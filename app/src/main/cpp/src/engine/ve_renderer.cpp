#include "ve_renderer.hpp"
#include "ve_imgui.hpp"

#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {

    VeRenderer::VeRenderer(VeWindow& window, VeDevice& device): veWindow{window}, veDevice{device} {
        recreateSwapChain();
        createCommandBuffers();
    }
    VeRenderer::~VeRenderer() { freeCommandBuffers(); }

    void VeRenderer::recreateSwapChain() {
        auto extent = veWindow.getExtent();

        vkDeviceWaitIdle(veDevice.device());
        // veSwapChain = std::make_unique<VeSwapChain>(veDevice, extent);
        if(veSwapChain == nullptr){
            veSwapChain = std::make_unique<VeSwapChain>(veDevice, extent);
        }else{
            std::shared_ptr<VeSwapChain> oldSwapChain = std::move(veSwapChain);
            veSwapChain = std::make_unique<VeSwapChain>(veDevice, extent,oldSwapChain);
            if(!oldSwapChain->compareSwapFormats(*veSwapChain)){
                throw std::runtime_error("Swap chain image and depth format changed!");
            }
        }

    }
    void VeRenderer::createCommandBuffers() {
        commandBuffers.resize(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = veDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        if (vkAllocateCommandBuffers(veDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
   
    void VeRenderer::freeCommandBuffers(){
        vkFreeCommandBuffers(veDevice.device(), veDevice.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
    }
    VkCommandBuffer VeRenderer::beginFrame(){
        assert(!isFrameStarted && "Can't call beginFrame while frame is already in progress.");
        bool changeImGuiOrientation = false;
        if(veWindow.isOrientationChanged()){
            recreateSwapChain();
            changeImGuiOrientation = true;
        }
        auto  result = veSwapChain->acquireNextImage(&currentImageIndex);
        //If the surface has changed and is no longer compatible with the swap chain
        //we need to recreate the swap chain
        if(result == VK_ERROR_OUT_OF_DATE_KHR){
            recreateSwapChain();
            return nullptr;
        }
        if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        isFrameStarted = true;
        auto commandBuffer = getCurrentCommandBuffer();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
            throw std::runtime_error("failed to begin recording command buffer!");
        }
        if(changeImGuiOrientation){
            auto extent = veWindow.getExtent();
//            ImGui_ImplVulkan_HandleRotation((int)extent.width, (int)extent.height,  commandBuffer);
//            ImGui_ImplVulkan_RecreateSwapchain((int)extent.width, (int)extent.height);
        }
        return commandBuffer;
    }
    void VeRenderer::endFrame(){
        assert(isFrameStarted && "Can't call endFrame while frame is not in progress.");
        auto commandBuffer = getCurrentCommandBuffer();
        if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
            throw std::runtime_error("failed to record command buffer!");
        }
        auto result = veSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR){
            recreateSwapChain();
        }else if (result == VK_SUBOPTIMAL_KHR) {
            veWindow.setOrientationChanged(true);
        }else if(result != VK_SUCCESS){
            throw std::runtime_error("failed to submit command buffer!");
        }
        isFrameStarted = false;
        currentFrameIndex = (currentFrameIndex + 1) % VeSwapChain::MAX_FRAMES_IN_FLIGHT;
    }
    void VeRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer){
        assert(isFrameStarted && "Can't begin render pass when frame is not in progress.");
        assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame.");
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = veSwapChain->getRenderPass();
        renderPassInfo.framebuffer = veSwapChain->getFrameBuffer(currentImageIndex);
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = veSwapChain->getSwapChainExtent();
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        //dynamic viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(veSwapChain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(veSwapChain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, veSwapChain->getSwapChainExtent()};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }
    void VeRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer){
        assert(isFrameStarted && "Can't end render pass when frame is not in progress.");
        assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame.");
        vkCmdEndRenderPass(commandBuffer);
    }
    void VeRenderer::resetWindow(){
        recreateSwapChain();
    }
}
