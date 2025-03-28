#pragma once
#include "ve_device.hpp"

#include <android/asset_manager.h>
#include <vulkan/vulkan.h>

#include <string>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace ve{
    class VeTexture{
        public:
            VeTexture(VeDevice& device, AAssetManager* assetManager, const std::string& albedoPath);
            ~VeTexture();
            VeTexture(const VeTexture&) = delete;
            VeTexture& operator=(const VeTexture&) = delete;
            VeTexture(VeTexture&&) = delete;
            VeTexture& operator=(VeTexture&&) = delete;

            VkImageView getImageView() const { return textureImageView; }
            VkSampler getSampler() const { return textureSampler; }

            VkImageLayout getLayout() const { return textureLayout; } // same for both albedo and normal
        private:
            void createTextureImage(AAssetManager * assetManager, VkFormat textureFormat, const std::string& path);
            void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels);
            void generateMipMaps(int mipLevels, int texWidth, int texHeight, VkFormat textureFormat);

            VeDevice& veDevice;
            //albedo texture map
            VkImage textureImage;
            VkDeviceMemory textureImageMemory;
            VkSampler textureSampler;
            VkImageView textureImageView;

            VkImageLayout textureLayout; //same for both albedo and normal
    };
}