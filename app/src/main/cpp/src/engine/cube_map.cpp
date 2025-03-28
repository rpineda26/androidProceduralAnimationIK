#include "cube_map.hpp"
#include "utility.hpp"
#include "debug.hpp"
#include <stb_image.h>

#include <iostream>
namespace ve{
    CubeMap::CubeMap(VeDevice& veDevice, bool nearestFilter): 
        device(veDevice),
        nearestFilter{nearestFilter}, 
        mipLevels(1),
        width(0), 
        height(0), 
        bytesPerPixel(0), 
        srgb(false),
        sampler(VK_NULL_HANDLE),
        imageView(VK_NULL_HANDLE),
        imageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
        image(VK_NULL_HANDLE),
        deviceMemory(VK_NULL_HANDLE) {
    }
    CubeMap::~CubeMap() {
        LOGI("Destroying CubeMap");
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(device.device(), image, nullptr);
        }
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device.device(), imageView, nullptr);
        }
        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device.device(), sampler, nullptr);
        }
        if (deviceMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device.device(), deviceMemory, nullptr);
        }
    }
    // In cube_map.cpp:
    CubeMap::CubeMap(CubeMap&& other) noexcept : 
        device(other.device),
        nearestFilter(other.nearestFilter),
        mipLevels(other.mipLevels),
        width(other.width),
        height(other.height),
        bytesPerPixel(other.bytesPerPixel),
        srgb(other.srgb),
        fileNames(std::move(other.fileNames)),
        format(other.format),
        imageLayout(other.imageLayout) {
        LOGI("CubeMap Move constructor");
        // Move Vulkan resources
        image = other.image;
        imageView = other.imageView;
        sampler = other.sampler;
        deviceMemory = other.deviceMemory;

        // Null out the original's handles to prevent double deletion
        other.image = VK_NULL_HANDLE;
        other.imageView = VK_NULL_HANDLE;
        other.sampler = VK_NULL_HANDLE;
        other.deviceMemory = VK_NULL_HANDLE;
    }

    CubeMap& CubeMap::operator=(CubeMap&& other) noexcept {
        if (this != &other) {
            // Clean up existing resources
            vkDestroyImage(device.device(), image, nullptr);
            vkDestroyImageView(device.device(), imageView, nullptr);
            vkDestroySampler(device.device(), sampler, nullptr);
            vkFreeMemory(device.device(), deviceMemory, nullptr);
        
            // Copy basic members
            nearestFilter = other.nearestFilter;
            mipLevels = other.mipLevels;
            width = other.width;
            height = other.height;
            bytesPerPixel = other.bytesPerPixel;
            srgb = other.srgb;
            fileNames = std::move(other.fileNames);
            format = other.format;
            imageLayout = other.imageLayout;
        
            // Move Vulkan resources
            image = other.image;
            imageView = other.imageView;
            sampler = other.sampler;
            deviceMemory = other.deviceMemory;
            
            // Null out the original's handles
            other.image = VK_NULL_HANDLE;
            other.imageView = VK_NULL_HANDLE;
            other.sampler = VK_NULL_HANDLE;
            other.deviceMemory = VK_NULL_HANDLE;
        }
        return *this;
    }
    bool CubeMap::init(AAssetManager *assetManager, const std::vector<std::string>& filePaths, bool srgb, bool flip){
        bool success = true;
        stbi_set_flip_vertically_on_load(flip);
        fileNames = filePaths;
        this->srgb = srgb;
        success = create(assetManager);
        LOGI("CubeMap created");
        return success;
    }
    void CubeMap::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout){
        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = CUBE_MAP_FACE_COUNT;
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }else{
            throw std::invalid_argument("unsupported layout transition!");
        }
        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        device.endSingleTimeCommands(commandBuffer);
        imageLayout = newLayout;
    }
    void CubeMap::createImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties){
        this->format = format;
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = CUBE_MAP_FACE_COUNT;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        if(vkCreateImage(device.device(), &imageInfo, nullptr, &image) != VK_SUCCESS){
            throw std::runtime_error("failed to create image!");
        }
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device.device(), image, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);
        if(vkAllocateMemory(device.device(), &allocInfo, nullptr, &deviceMemory) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate image memory!");
        }
        vkBindImageMemory(device.device(), image, deviceMemory, 0);
    }

    void CubeMap::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory){
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if(vkCreateBuffer(device.device(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create buffer!");
        }
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device.device(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);
        if(vkAllocateMemory(device.device(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate buffer memory!");
        }
        vkBindBufferMemory(device.device(), buffer, bufferMemory, 0);
    }
    bool CubeMap::create(AAssetManager* assetManager){
        VkDeviceSize layerSize;
        VkDeviceSize imageSize;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        void* data;
        uint64_t memAddress;
        stbi_uc* pixels;

        for(int i=0; i<CUBE_MAP_FACE_COUNT; i++){
            //req_comp = STBI_rgb_alpha == 4
            std::string fullPath = std::string(ENGINE_DIR) + fileNames[i];
            std::vector<uint8_t> imageData = ve::loadBinaryFileToVector(fileNames[i].c_str(), assetManager);
            if(imageData.empty()){
                LOGE("Failed to load texture image!");
                return false;
            }
            pixels = stbi_load_from_memory(imageData.data(),imageData.size(), &width, &height, &bytesPerPixel, STBI_rgb_alpha);
            if(pixels==nullptr){
                LOGE("Failed to load image to memory %s", stbi_failure_reason());
            }
            if(i==0){
                layerSize = width * height * 4;
                imageSize = layerSize * CUBE_MAP_FACE_COUNT;
                createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                            stagingBuffer, stagingBufferMemory);
                vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
                memAddress = reinterpret_cast<uint64_t>(data);
            }
            memcpy(reinterpret_cast<void*>(memAddress), static_cast<void*>(pixels), static_cast<size_t>(layerSize));
            stbi_image_free(pixels);
            memAddress += layerSize;
        }
        vkUnmapMemory(device.device(), stagingBufferMemory);
        VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        createImage(format, 
                    VK_IMAGE_TILING_OPTIMAL, 
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                );
        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        device.copyBufferToImage(stagingBuffer, 
                                image, 
                                static_cast<uint32_t>(width), 
                                static_cast<uint32_t>(height), 
                                CUBE_MAP_FACE_COUNT
                            );
        transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
        vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

        //create sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = nearestFilter ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
        samplerInfo.minFilter = nearestFilter ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0;
        samplerInfo.maxLod = static_cast<float>(mipLevels);
        samplerInfo.mipLodBias = 0;
        if(vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS){
            throw std::runtime_error("failed to create cube map texture sampler!");
        }
        //imageView
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = CUBE_MAP_FACE_COUNT;
        if(vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS){
            throw std::runtime_error("failed to create texture image view!");
        }
        LOGI("Successfully loaded cubemaps!");
        return true;
    }
}