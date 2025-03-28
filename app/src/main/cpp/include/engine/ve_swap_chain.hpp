#pragma once

#include "ve_device.hpp"
#include "ve_imgui.hpp"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>
#include <memory>
namespace ve {

class VeSwapChain {
 public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  VeSwapChain(VeDevice &deviceRef, VkExtent2D windowExtent);
  VeSwapChain(VeDevice &deviceRef, VkExtent2D windowExtent, std::shared_ptr<VeSwapChain> previous);
  ~VeSwapChain();

  VeSwapChain(const VeSwapChain &) = delete;
  VeSwapChain& operator=(const VeSwapChain &) = delete;

  VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
  VkRenderPass getRenderPass() { return renderPass; }
  VkImageView getImageView(int index) { return swapChainImageViews[index]; }
  size_t imageCount() { return swapChainImages.size(); }
  VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
  VkExtent2D getSwapChainExtent() { return swapChainExtent; }
  uint32_t width() { return swapChainExtent.width; }
  uint32_t height() { return swapChainExtent.height; }
  Orientation getOrientation() {return orientation;}

  float extentAspectRatio() {
    return fmin(static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), static_cast<float>(swapChainExtent.height) / static_cast<float>(swapChainExtent.width));
  }
  VkFormat findDepthFormat();

  VkResult acquireNextImage(uint32_t *imageIndex);
  VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

  bool compareSwapFormats(const VeSwapChain &swapChain) const {
    return swapChainImageFormat == swapChain.swapChainImageFormat &&
           swapChainDepthFormat == swapChain.swapChainDepthFormat;
  }
  VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
  VkSurfaceTransformFlagBitsKHR getPreTransformFlag() const { return preTransformFlag; }

 private:
  void init();
  void createSwapChain();
  void createImageViews();
  void createDepthResources();
  void createRenderPass();
  void createFramebuffers();
  void createSyncObjects();
  void determineOrientation(const VkSurfaceCapabilitiesKHR& capabilities);

  // Helper functions
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  VkFormat swapChainImageFormat;
  VkFormat swapChainDepthFormat;
  VkExtent2D swapChainExtent;

  std::vector<VkFramebuffer> swapChainFramebuffers;
  VkRenderPass renderPass;

  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemorys;
  std::vector<VkImageView> depthImageViews;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;

  VeDevice &device;
  VkExtent2D windowExtent;

  VkSwapchainKHR swapChain;
  std::shared_ptr<VeSwapChain> oldSwapChain;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  std::vector<VkFence> imagesInFlight;
  VkSurfaceTransformFlagBitsKHR preTransformFlag;
  size_t currentFrame = 0;
  Orientation orientation;
};

}  // namespace ve