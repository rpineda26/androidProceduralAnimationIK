#pragma once

#include "ve_window.hpp"
// std lib headers
#include <string>
#include <vector>

namespace ve {

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
  uint32_t graphicsFamily;
  uint32_t presentFamily;
  bool graphicsFamilyHasValue = false;
  bool presentFamilyHasValue = false;
  bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

class VeDevice {
 public:
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

  VeDevice(VeWindow &window);
  ~VeDevice();

  // Not copyable or movable
  VeDevice(const VeDevice &) = delete;
  VeDevice& operator=(const VeDevice &) = delete;
  VeDevice(VeDevice &&) = delete;
  VeDevice& operator=(VeDevice &&) = delete;

  VkCommandPool getCommandPool() { return commandPool; }
  VkInstance getInstance() { return instance; }
  VkDevice device() { return device_; }
  VkPhysicalDevice getPhysicalDevice() { return physicalDevice; }
  VkSurfaceKHR surface() { return surface_; }
  VkQueue graphicsQueue() { return graphicsQueue_; }
  VkQueue presentQueue() { return presentQueue_; }
  uint32_t graphicsQueueFamilyIndex() { return findPhysicalQueueFamilies().graphicsFamily; }
  SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
  QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
  VkFormat findSupportedFormat(
      const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

  // Buffer Helper Functions
  void createBuffer(
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkBuffer &buffer,
      VkDeviceMemory &bufferMemory);
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void copyBufferToImage(
      VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

  void createImageWithInfo(
      const VkImageCreateInfo &imageInfo,
      VkMemoryPropertyFlags properties,
      VkImage &image,
      VkDeviceMemory &imageMemory);
    void resetWindow();
    VkPhysicalDeviceProperties properties;
    
  
 private:
  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createCommandPool();

  // helper functions
  bool isDeviceSuitable(VkPhysicalDevice device);
  std::vector<const char *> getRequiredExtensions();
  bool checkValidationLayerSupport();
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
  void hasGflwRequiredInstanceExtensions();
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VeWindow &window;
  VkCommandPool commandPool;

  VkDevice device_;
  VkSurfaceKHR surface_;
  VkQueue graphicsQueue_;
  VkQueue presentQueue_;

  const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
  //add vk_KHR_portability_subset to the list of required extensions for macOS
  bool setMACOSExtensionSupport();
  #ifdef MACOS
    std::vector<const char *>deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset", VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME};
  #else
    std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  #endif
  };

}  // namespace ve