#pragma once
#include "ve_device.hpp"
#include "buffer.hpp"

#include <android/asset_manager.h>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif


namespace ve{
    class CubeMap{
        public:
            CubeMap(VeDevice& veDevice, bool nearestFilter);
            ~CubeMap();
            CubeMap(const CubeMap&) = delete; // Disable copy constructor
            CubeMap& operator=(const CubeMap&) = delete; // Disable copy assignment
            CubeMap(CubeMap&& other) noexcept;
            CubeMap& operator=(CubeMap&& other) noexcept;

            bool init(AAssetManager *assetManager, const std::vector<std::string>& filePaths, bool srgb, bool flip=true);

            int getWidht() const { return width; }
            int getHeight() const { return height; }
            VkImageView getImageView() const { return imageView; }
            VkSampler getSampler() const { return sampler; }
            VkImageLayout getImageLayout() const { return imageLayout; }

            static constexpr bool USE_SRGB = true;
            static constexpr bool USE_NORM = false;

        private:
            bool create(AAssetManager *assetManager);
            void createImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
            void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
            void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);


            //attributes
            static constexpr int CUBE_MAP_FACE_COUNT = 6;
            VeDevice& device;
            VkSampler sampler;
            VkImageView imageView;
            VkImageLayout imageLayout;
            VkDescriptorImageInfo descriptorImageInfo;
            VkImage image;
            VkDeviceMemory deviceMemory;
            VkFormat format;
            std::vector<std::string> fileNames;
            unsigned int mipLevels;
            int width;
            int height;
            int bytesPerPixel;
            bool srgb;
            bool nearestFilter;
    };
}