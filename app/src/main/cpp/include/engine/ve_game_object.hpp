#pragma once

#include "ve_model.hpp"
#include "ve_device.hpp"
#include "ve_descriptors.hpp"
#include "cube_map.hpp"
#include "debug.hpp"

#include <android/asset_manager.h>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>
#include <cstring>
namespace ve{
    struct TransformComponent{
        glm::vec3 translation{0.0f,0.0f,0.0f};
        glm::vec3 scale{1.0f,1.0f,1.0f};
//        glm::vec3 rotation = glm::vec3{glm::radians(180.0f),0.0f,0.0f};
        glm::vec3 rotation = glm::vec3{0.0f,0.0f,0.0f};
        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };
    struct PointLightComponent{
        float lightIntensity = 1.0f;
    };
    struct CubeMapComponent{
        std::unique_ptr<CubeMap> cubeMap;
        VkDescriptorSet descriptorSet;
        std::unique_ptr<VeDescriptorSetLayout> descriptorSetLayout;
    };
    struct AnimationComponent{
        std::unique_ptr<VeDescriptorSetLayout> animationSetLayout;
        std::vector<VkDescriptorSet> animationDescriptorSets;
        std::vector<std::unique_ptr<VeBuffer>> shaderJointsBuffer;
    };

    class VeGameObject { 
        public:
            //user defined types
            using id_t = unsigned int;
            using Map = std::unordered_map<id_t, VeGameObject>;
            //instantiation of game Object
            static VeGameObject createGameObject(){ 
                static id_t currentId = 0;
                LOGI("Creating game object with id: %d", currentId);
                return VeGameObject{currentId++}; 
            }
            //instantiation of point light
            static VeGameObject createPointLight(float intensity=1.0f, float radius=0.1f, glm::vec3 color=glm::vec3(1.0f));
            //instantiation of cube map
            static VeGameObject createCubeMap(VeDevice& device, AAssetManager* assetManager, const std::vector<std::string>& faces, VeDescriptorPool& descriptorPool);
            //instantiation of game object with animation
            static VeGameObject createAnimatedObject(VeDevice& device, VeDescriptorPool& descriptorPool, std::shared_ptr<VeModel> veModel);
            void updateAnimation(float deltaTime, int frameCounter, int frameIndex);

            VeGameObject(const VeGameObject&) = delete;
            VeGameObject& operator=(const VeGameObject&) = delete;
            VeGameObject(VeGameObject&&) = default;
            VeGameObject& operator=(VeGameObject&&) = default;
            
            //attributes
            glm::vec3  color{1.0f};
            TransformComponent transform{};
            //optional attributes
            std::shared_ptr<VeModel> model{};
            std::unique_ptr<PointLightComponent> lightComponent = nullptr;
            std::unique_ptr<CubeMapComponent> cubeMapComponent = nullptr;
            std::unique_ptr<AnimationComponent> animationComponent = nullptr;

            //getters and setters
            id_t getId() { return id; }
            char* getTitle(){
                return title;
            }
            void setTitle(const char* newTitle){
                strncpy(title, newTitle, sizeof(title));
            }
            void setTextureIndex(uint32_t index){
                textureIndex = index;
            }
            void setNormalIndex(uint32_t index){
                normalIndex = index;
            }
            void setSpecularIndex(uint32_t index){
                specularIndex = index;
            }
            uint32_t getTextureIndex(){
                return textureIndex;
            }
            uint32_t getNormalIndex(){
                return normalIndex;
            }
            uint32_t getSpecularIndex(){
                return specularIndex;
            }
            void setSmoothness(float value){
                smoothness = value;
            }
            float getSmoothness(){
                return smoothness;
            }
        private:
            //instantiation of VeGameobject is only allowed through createGameObject to 
            //make sure id is unique (incrementing)
            VeGameObject(id_t objId): id{objId} {}
            id_t id;
            char title[26]; 
            uint32_t textureIndex = -1;
            uint32_t normalIndex = -1;
            uint32_t specularIndex = -1;
            float smoothness = 0.0f;

    };

}