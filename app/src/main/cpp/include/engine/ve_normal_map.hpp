#pragma once
#include "ve_device.hpp"

#include <android/asset_manager.h>
#include <vulkan/vulkan.h>

#include <string>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace ve{
    class VeNormal{
        public:
            VeNormal(VeDevice& device, const std::string& normalPath, AAssetManager *assetManager);
            ~VeNormal();
            VeNormal(const VeNormal&) = delete;
            VeNormal& operator=(const VeNormal&) = delete;
            VeNormal(VeNormal&&) = delete;
            VeNormal& operator=(VeNormal&&) = delete;


            VkImageView getNormalImageView() const { return normalImageView; }
            VkSampler getNormalSampler() const { return normalSampler; }
            VkImageLayout getLayout() const { return textureLayout; } // same for both albedo and normal
        private:
            void createTextureImageNormal(VkFormat textureFormat, const std::string& path, AAssetManager *assetManager);
            void transitionImageLayoutNormal(VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels);
            void generateMipMapsNormal(int mipLevels, int texWidth, int texHeight, VkFormat textureFormat);
            VeDevice& veDevice;

            //normal map
            VkImage normalImage;
            VkDeviceMemory normalImageMemory;
            VkSampler normalSampler;
            VkImageView normalImageView;

            VkImageLayout textureLayout; //same for both albedo and normal
    };
}