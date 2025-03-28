#include "ve_game_object.hpp"
#include "debug.hpp"
#include <iostream>

namespace ve{
    glm::mat4 TransformComponent::mat4() {
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.x);
        const float s2 = glm::sin(rotation.x);
        const float c1 = glm::cos(rotation.y);
        const float s1 = glm::sin(rotation.y);
        return glm::mat4{
            {
                scale.x * (c1 * c3 + s1 * s2 * s3),
                scale.x * (c2 * s3),
                scale.x * (c1 * s2 * s3 - c3 * s1),
                0.0f,
            },
            {
                scale.y * (c3 * s1 * s2 - c1 * s3),
                scale.y * (c2 * c3),
                scale.y * (c1 * c3 * s2 + s1 * s3),
                0.0f,
            },
            {
                scale.z * (c2 * s1),
                scale.z * (-s2),
                scale.z * (c1 * c2),
                0.0f,
            },
            {translation.x, translation.y, translation.z, 1.0f}
        };
    }
    glm::mat3 TransformComponent::normalMatrix() {
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.x);
        const float s2 = glm::sin(rotation.x);
        const float c1 = glm::cos(rotation.y);
        const float s1 = glm::sin(rotation.y);
        const glm::vec3 invScale = 1.0f / scale;
        return glm::mat3{
            {
                invScale.x * (c1 * c3 + s1 * s2 * s3),
                invScale.x * (c2 * s3),
                invScale.x * (c1 * s2 * s3 - c3 * s1),
            },
            {
                invScale.y * (c3 * s1 * s2 - c1 * s3),
                invScale.y * (c2 * c3),
                invScale.y * (c1 * c3 * s2 + s1 * s3),
            },
            {
                invScale.z * (c2 * s1),
                invScale.z * (-s2),
                invScale.z * (c1 * c2),
            },
        };
    }
    VeGameObject VeGameObject::createPointLight(float intensity, float radius, glm::vec3 color){
        VeGameObject pointLight = VeGameObject::createGameObject();
        pointLight.lightComponent = std::make_unique<PointLightComponent>();
        pointLight.lightComponent->lightIntensity = intensity;
        pointLight.transform.scale.x = radius;
        pointLight.color = color;
        return pointLight;
    }
    VeGameObject VeGameObject::createCubeMap(VeDevice& device, AAssetManager* assetManager, const std::vector<std::string>& faces, VeDescriptorPool& descriptorPool){
        static constexpr int VERTEX_COUNT = 36;
        VeGameObject cubeObj = VeGameObject::createGameObject();
        
        glm::vec3 cubeMapVertices[VERTEX_COUNT] = {
            // Positions (X, Y, Z)
            {-1.0f,  1.0f, -1.0f}, // Top-left front
            {-1.0f, -1.0f, -1.0f}, // Bottom-left front
            { 1.0f, -1.0f, -1.0f}, // Bottom-right front
            { 1.0f, -1.0f, -1.0f}, // Bottom-right front
            { 1.0f,  1.0f, -1.0f}, // Top-right front
            {-1.0f,  1.0f, -1.0f}, // Top-left front
        
            {-1.0f, -1.0f,  1.0f}, // Bottom-left back
            {-1.0f, -1.0f, -1.0f}, // Bottom-left front
            {-1.0f,  1.0f, -1.0f}, // Top-left front
            {-1.0f,  1.0f, -1.0f}, // Top-left front
            {-1.0f,  1.0f,  1.0f}, // Top-left back
            {-1.0f, -1.0f,  1.0f}, // Bottom-left back
        
            {1.0f, -1.0f, -1.0f},  // Bottom-right front
            {1.0f, -1.0f,  1.0f},  // Bottom-right back
            {1.0f,  1.0f,  1.0f},  // Top-right back
            {1.0f,  1.0f,  1.0f},  // Top-right back
            {1.0f,  1.0f, -1.0f},  // Top-right front
            {1.0f, -1.0f, -1.0f},  // Bottom-right front
        
            {-1.0f, -1.0f,  1.0f}, // Bottom-left back
            {-1.0f,  1.0f,  1.0f}, // Top-left back
            { 1.0f,  1.0f,  1.0f}, // Top-right back
            { 1.0f,  1.0f,  1.0f}, // Top-right back
            { 1.0f, -1.0f,  1.0f}, // Bottom-right back
            {-1.0f, -1.0f,  1.0f}, // Bottom-left back
        
            {-1.0f,  1.0f, -1.0f}, // Top-left front
            { 1.0f,  1.0f, -1.0f}, // Top-right front
            { 1.0f,  1.0f,  1.0f}, // Top-right back
            { 1.0f,  1.0f,  1.0f}, // Top-right back
            {-1.0f,  1.0f,  1.0f}, // Top-left back
            {-1.0f,  1.0f, -1.0f}, // Top-left front
        
            {-1.0f, -1.0f, -1.0f}, // Bottom-left front
            {-1.0f, -1.0f,  1.0f}, // Bottom-left back
            { 1.0f, -1.0f, -1.0f}, // Bottom-right front
            { 1.0f, -1.0f, -1.0f}, // Bottom-right front
            {-1.0f, -1.0f,  1.0f}, // Bottom-left back
            { 1.0f, -1.0f,  1.0f}, // Bottom-right back
        };
        

         
        //load vertex data with the model class
        cubeObj.model = VeModel::createCubeMap(device, cubeMapVertices);
        //load the texture data
        auto cubeMap = CubeMap(device, true);
        bool srgb = true;
        bool flip = false;
        if(cubeMap.init(assetManager, faces, srgb, flip)){
            cubeObj.cubeMapComponent = std::make_unique<CubeMapComponent>();
            cubeObj.cubeMapComponent->cubeMap = std::make_unique<CubeMap>(std::move(cubeMap));
        }else{
            throw std::runtime_error("Failed to load cubemap");
        }
        //create descriptor set layout
        cubeObj.cubeMapComponent->descriptorSetLayout = VeDescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();
        //create descriptor set
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = cubeObj.cubeMapComponent->cubeMap->getImageLayout();
        imageInfo.imageView = cubeObj.cubeMapComponent->cubeMap->getImageView();
        imageInfo.sampler = cubeObj.cubeMapComponent->cubeMap->getSampler();
        VeDescriptorWriter(*cubeObj.cubeMapComponent->descriptorSetLayout, descriptorPool)
            .writeImage(0, &imageInfo, 1)
            .build(cubeObj.cubeMapComponent->descriptorSet);
        LOGI("Cubemap created");
        return cubeObj;
    }
}