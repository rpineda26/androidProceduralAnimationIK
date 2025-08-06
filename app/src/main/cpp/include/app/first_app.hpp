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
#include "model_manager.hpp"
#include "joint_editor.hpp"
#include "frame_info.hpp"
#include "shadow_manager.hpp"
#include "animation_sequencer.hpp"
//render systems
#include "pbr_render_system.hpp"
#include "point_light_system.hpp"
#include "outline_highlight_system.hpp"
#include "cube_map_system.hpp"
#include "skeleton_system.hpp"
#include "shadow_render_system.hpp"

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
            std::string changeModel(std::string name);
            InputHandler inputHandler{};
            void toggleDogModelList() {engineInfo.showModelsList = !engineInfo.showModelsList;}
            void toggleChangeAnimationList() {engineInfo.changeAnimationList = !engineInfo.changeAnimationList;}
            void toggleEditJointMode() {engineInfo.showEditJointMode = !engineInfo.showEditJointMode;}
            void toggleOutlignHighlight() {engineInfo.showOutlignHighlight = !engineInfo.showOutlignHighlight;}
            bool toggleAnimationPlayer(bool value);
            void pauseAnimation();
            void resumeAnimation();
            bool isAnimationPlaying();
            void jumpForward();
            void jumpBackward();
            float getAnimationDuration();
            void setAnimationTime(float time);
            float getCurrentAnimationTime();
            void setModelMode(bool value);
            void cleanup();
            void cleanupSurface();

    private:
            void loadGameObjects();
            void loadTextures();
            int getNumLights();
            void renderDogModelList();
            void updateModelLoadingStatus();
            void drawSimpleSpinner(const std::string& loadingText);
            void renderChangeAnimationList();
            void renderShadowFace(VkCommandBuffer commandBuffer, int lightIndex, int faceIndex, const glm::mat4& viewProjMatrix, FrameInfo frameInfo, PointLight lights[10]);


        //core engine components
            std::unique_ptr<VeWindow> veWindow;
            std::unique_ptr<VeDevice> veDevice;
            std::unique_ptr<VeRenderer> veRenderer;
            std::unique_ptr<AAssetManager,AAssetManagerDeleter> assetManager;
            std::unique_ptr<ModelManager> g_modelManager;

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
            //shadowdescriptor
            std::unique_ptr<VeDescriptorSetLayout> shadowSetLayout;
            VkDescriptorSet shadowDescriptorSet = VK_NULL_HANDLE;
            std::vector<VkDescriptorImageInfo> shadowMapInfos;

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
            std::unique_ptr<ShadowRenderSystem> shadowRenderSystem;

            //scene entities
            VeGameObject::Map gameObjects;
            EngineInfo engineInfo;
            AnimationSequencer animationSequencer;
            std::unique_ptr<ShadowManager> shadowManager;

    };
}