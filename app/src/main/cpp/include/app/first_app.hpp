#pragma once
//project components
#include "ve_window.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include "ve_renderer.hpp"
#include "ve_descriptors.hpp"
#include "ve_texture.hpp"
#include "ve_normal_map.hpp"
#include "buffer.hpp"
#include "input_handler.hpp"

//render systems
#include "pbr_render_system.hpp"
#include "point_light_system.hpp"
#include "outline_highlight_system.hpp"
#include "cube_map_system.hpp"
#include "skeleton_system.hpp"

//core libraries
#include <android/asset_manager.h>
#include <vulkan/vulkan.h>

//std
#include <chrono>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ve {
    class FirstApp{
        struct EngineInfo{
            float frameTime{0.0f};
            float elapsedTime{0.0f};
            std::chrono::time_point<std::chrono::steady_clock,std::chrono::duration<long long, std::ratio<1,1000000000>>>
                currentTime = std::chrono::high_resolution_clock::now();;
            int numLights{0};
            int frameCount{0};
            int cubeMapIndex{3};
            int animatedObjIndex{1};
            int selectedObject{1};
            int selectedJointIndex{-1};
            bool showOutlignHighlight{false};
            bool engineInitialized{false};
            VeCamera camera{};
            VeGameObject viewerObject = VeGameObject::createGameObject();
        };

        struct AAssetManagerDeleter {
            void operator()(AAssetManager* newAssetManager) {
                // AAssetManager is typically managed by the Android system
                // So we don't actually delete it
                // Just set the pointer to nnullptrullptr if needed
                newAssetManager = nullptr;
            }
        };

        public:

            FirstApp() = default;
            ~FirstApp();
            FirstApp(const FirstApp&) = delete;
            FirstApp& operator=(const FirstApp&) = delete;
            void run();
            void init();
            void reset(ANativeWindow* newWindow, AAssetManager* newManager);

            //getters & setters
            bool isInitialized() const {return engineInfo.engineInitialized;}
            void setInitialized(bool value) {engineInfo.engineInitialized = value;}
            void controlCamera();

        public:
            InputHandler inputHandler{};
             
        private:
            void loadGameObjects();
            void loadTextures();
            int getNumLights();

            //core engine components
            std::unique_ptr<VeWindow> veWindow;
            std::unique_ptr<VeDevice> veDevice;
            std::unique_ptr<VeRenderer> veRenderer;
            std::unique_ptr<AAssetManager,AAssetManagerDeleter> assetManager;

            //Gui
            VkRenderPass imGuiRenderPass = VK_NULL_HANDLE;
            VkDescriptorPool imGuiPool = VK_NULL_HANDLE;

            //descriptor resources
            std::unique_ptr<VeDescriptorPool> globalPool{};
            //uniform buffer descriptor (view data)
            std::unique_ptr<VeDescriptorSetLayout> globalSetLayout;
            std::vector<VkDescriptorSet> globalDescriptorSets;
            std::vector<std::unique_ptr<VeBuffer>> uniformBuffers;
            //texture descriptor
            std::unique_ptr<VeDescriptorSetLayout> textureSetLayout;
            VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;

            //texture info
            std::vector<std::unique_ptr<VeTexture>> textures;
            std::vector<std::unique_ptr<VeNormal>> normalMaps;
            std::vector<std::unique_ptr<VeNormal>> specularMaps;
            std::vector<VkDescriptorImageInfo> textureInfos;
            std::vector<VkDescriptorImageInfo> normalMapInfos;
            std::vector<VkDescriptorImageInfo> specularMapInfos;

            //render systems
            std::unique_ptr<PbrRenderSystem> pbrRenderSystem;
            std::unique_ptr<PointLightSystem> pointLightSystem;
            std::unique_ptr<OutlineHighlightSystem> outlineHighlightSystem;
            std::unique_ptr<CubeMapRenderSystem> cubeMapRenderSystem;
            std::unique_ptr<SkeletonSystem> skeletonSystem;

            //scene entities
            VeGameObject::Map gameObjects;
            EngineInfo engineInfo;

    };
}