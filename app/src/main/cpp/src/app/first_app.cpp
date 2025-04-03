#include "first_app.hpp"
//#include "input_controller.hpp"
#include "ve_imgui.hpp"
#include "utility.hpp"
#include "debug.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {
    //cleanup
    FirstApp::~FirstApp() {
        vkDeviceWaitIdle(veDevice->device());
        VeImGui::cleanUpImGui();
        vkDestroyRenderPass(veDevice->device(), imGuiRenderPass, nullptr);
        vkDestroyDescriptorPool(veDevice->device(), imGuiPool, nullptr);
        cleanupPreloadedModels();
        engineInfo.engineInitialized = false;
    }
    void FirstApp::init(){
        if(!(veWindow && veDevice && veRenderer && assetManager)){
            LOGE("FirstApp::init() called before having both ANativeWindow and AAssetManager");
            return;
        }
        //setup descriptor pools
        globalPool = VeDescriptorPool::Builder(*veDevice)
                .setMaxSets(20000)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
        #ifdef MACOS
                .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        #endif
                .build();
        //Dear ImGui DescriptorPool
        imGuiPool = VeImGui::createDescriptorPool(veDevice->device());
        //load assets
        preLoadModels(*veDevice, assetManager.get());
        loadGameObjects();
        loadTextures();
        //init uniform buffers
        uniformBuffers.resize(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < uniformBuffers.size(); i++){
            uniformBuffers[i] = std::make_unique<VeBuffer>(
                *veDevice,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                veDevice->properties.limits.minUniformBufferOffsetAlignment
            );
            uniformBuffers[i]->map();
        }
        //init descriptor resources
        //global ubo descriptor
        globalSetLayout = VeDescriptorSetLayout::Builder(*veDevice)
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
                .build();
        globalDescriptorSets.resize(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < globalDescriptorSets.size(); i++){
            auto bufferInfo = uniformBuffers[i]->descriptorInfo();
            VeDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }
        //texture descriptor
        textureSetLayout = VeDescriptorSetLayout::Builder(*veDevice)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
                .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
                .build();
        VeDescriptorWriter(*textureSetLayout, *globalPool)
            .writeImage(0, textureInfos.data(),3)
            .writeImage(1, normalMapInfos.data(),3)
            .writeImage(2, specularMapInfos.data(),3)
            .build(textureDescriptorSet);

        //init render systems
        pbrRenderSystem = std::make_unique<PbrRenderSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(),
            std::vector<VkDescriptorSetLayout>{globalSetLayout->getDescriptorSetLayout(), textureSetLayout->getDescriptorSetLayout(), gameObjects.at(engineInfo.animatedObjIndex).animationComponent->animationSetLayout->getDescriptorSetLayout()});
        pointLightSystem = std::make_unique<PointLightSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout());
        outlineHighlightSystem = std::make_unique<OutlineHighlightSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout());
        cubeMapRenderSystem = std::make_unique<CubeMapRenderSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(),
            std::vector<VkDescriptorSetLayout>{globalSetLayout->getDescriptorSetLayout(), gameObjects.at(engineInfo.cubeMapIndex).cubeMapComponent->descriptorSetLayout->getDescriptorSetLayout()});
        skeletonSystem = std::make_unique<SkeletonSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(), std::vector<VkDescriptorSetLayout>{globalSetLayout->getDescriptorSetLayout()});
        //edit starting camera setup
//        engineInfo.viewerObject.transform.translation ={-1.42281f,-10.1585,0.632};
//        engineInfo.viewerObject.transform.rotation = {-1.33747,1.56693f,0.0f};
        //camera controller
        //gametime
        //imgui
        imGuiRenderPass = VeImGui::createRenderPass(veDevice->device(), veRenderer->getSwapChainImageFormat(), veRenderer->getSwapChainDepthFormat());
        VeImGui::createImGuiContext(*veDevice, *veWindow, imGuiPool, imGuiRenderPass, VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        gameObjects.at(engineInfo.animatedObjIndex).model->animationManager->start(0);
        engineInfo.camera.getOrbitViewMatrix(gameObjects.at(engineInfo.animatedObjIndex).transform.translation);

        engineInfo.engineInitialized = true;
        LOGI("FirstApp initialized");
    }
    void FirstApp::reset(ANativeWindow *newWindow, AAssetManager *newManager) {
        assetManager.reset(newManager);
        if(engineInfo.engineInitialized){
            veWindow->resetWindow(newWindow);
            veDevice->resetWindow();
            veRenderer->resetWindow();
            LOGI("ANativeWindow and AAssetManager successfully reset.");
        }else{
            veWindow = std::make_unique<VeWindow>("Vulkan Engine", newWindow);
            veDevice = std::make_unique<VeDevice>(*veWindow);
            veRenderer = std::make_unique<VeRenderer>(*veWindow, *veDevice);
            LOGI("ANativeWindow and AAssetManager successfully initialized.");
        }
    }
    void FirstApp::run() {
        if(!isInitialized()) {
            LOGE("Attempting to render without calling FirstApp::init()");
            return;
        }
        //main loop
        //track time
        auto newTime = std::chrono::high_resolution_clock::now();
        engineInfo.frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - engineInfo.currentTime).count();
        engineInfo.currentTime = newTime;
        engineInfo.frameTime = glm::clamp(engineInfo.frameTime, 0.0001f, 0.1f); //clamp large frametimes
        engineInfo.elapsedTime += engineInfo.frameTime;

        //update objects based on input
//            inputController.inputLogic(veWindow.getGLFWWindow(), frameTime, gameObjects, viewerObject, selectedObject);
//        engineInfo.camera.setViewTarget(engineInfo.viewerObject.transform.translation, gameObjects.at(engineInfo.animatedObjIndex).transform.translation);
//        engineInfo.camera.setViewYXZ(engineInfo.viewerObject.transform.translation, engineInfo.viewerObject.transform.rotation);
        //update camera aspect ratio
        float aspect = veRenderer->getAspectRatio();
        // camera.setOrtho(-aspect, aspect, -0.9f, 0.9f, -1.0f, 1.0f);
        engineInfo.camera.setPerspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        engineInfo.camera.getOrbitViewMatrix(gameObjects.at(engineInfo.animatedObjIndex).transform.translation);
        engineInfo.camera.calculatePreRotationMatrix(veRenderer->getPreRotTransformFlag());



        //render frame
        if(auto commandBuffer = veRenderer->beginFrame()){
            //start new imgui frame
            VeImGui::initializeImGuiFrame();
//            ImGui::ShowDemoWindow();
            engineInfo.numLights = getNumLights();
            renderGameObjectDetails(gameObjects.at(engineInfo.animatedObjIndex), engineInfo.showOutlignHighlight, engineInfo.selectedJointIndex);
            //record frame data
            int frameIndex = veRenderer->getFrameIndex();
            FrameInfo frameInfo{frameIndex, engineInfo.frameTime, engineInfo.elapsedTime, commandBuffer, engineInfo.camera, globalDescriptorSets[frameIndex], gameObjects, engineInfo.animatedObjIndex, engineInfo.selectedJointIndex,  engineInfo.numLights, engineInfo.showOutlignHighlight};
            //update global UBO
            GlobalUbo globalUbo{};
            globalUbo.projection = engineInfo.camera.getProjectionMatrix();
            globalUbo.view = engineInfo.camera.getRotViewMatrix();
            globalUbo.inverseView = engineInfo.camera.getInverseMatrix();
            globalUbo.selectedLight = engineInfo.selectedObject;
            globalUbo.frameTime = engineInfo.frameTime;
            pointLightSystem->update(frameInfo, globalUbo);
            uniformBuffers[frameIndex]->writeToBuffer(&globalUbo);
            uniformBuffers[frameIndex]->flush();
            //update animation
            gameObjects.at(engineInfo.animatedObjIndex).updateAnimation(engineInfo.frameTime, engineInfo.frameCount, frameIndex);

            //render scene
            veRenderer->beginSwapChainRenderPass(commandBuffer);

            if(engineInfo.showOutlignHighlight) {
//                outlineHighlightSystem->renderGameObjects(frameInfo);
                skeletonSystem->renderJoints(frameInfo, {globalDescriptorSets[frameIndex]});
                skeletonSystem->renderJointConnections(frameInfo, {globalDescriptorSets[frameIndex]});
            }else
                pbrRenderSystem->renderGameObjects(frameInfo, /*shadowRenderSystem.getShadowDescriptorSet(frameIndex),*/ {globalDescriptorSets[frameIndex], textureDescriptorSet, gameObjects.at(engineInfo.animatedObjIndex).animationComponent->animationDescriptorSets[frameIndex]});
            pointLightSystem->render(frameInfo);
            cubeMapRenderSystem->renderGameObjects(frameInfo);

            VeImGui::renderImGuiFrame(commandBuffer);
            veRenderer->endSwapChainRenderPass(commandBuffer);
            veRenderer->endFrame();
        }
        engineInfo.frameCount++;
//        LOGI("Rendering frame: %d", engineInfo.frameCount);
    }
    void FirstApp::controlCamera(){
//        LOGI("FirstApp controlCamera called");
        inputHandler.processMovement(engineInfo.camera);
        InputHandler::CameraMovement sampleMovement = inputHandler.getCameraMovement();
//        LOGI("deltaX: %f, deltaY: %f, zoom: %f", sampleMovement.deltaX, sampleMovement.deltaY, sampleMovement.pinchScale);

    }

    void FirstApp::loadGameObjects() {
        auto fox = VeGameObject::createAnimatedObject(*veDevice, *globalPool, preLoadedModels["Fox"]);
//        auto fox = VeGameObject::createGameObject();
        fox.setTextureIndex(2);
        fox.setNormalIndex(2);
        fox.setSpecularIndex(2);
//        fox.model = preLoadedModels["Cute_Demon"];
        // fox.transform.translation = {0.5f, 0.5f, 0.0f};
        fox.transform.scale = {0.03f, 0.03f, 0.03f};
        // fox.color = {128.0f, 228.1f, 229.1f}; //cyan
        fox.setTitle("Fox");
        gameObjects.emplace(fox.getId(),std::move(fox));
        engineInfo.animatedObjIndex = fox.getId();

        // auto man = VeGameObject::createGameObject();
        // man.setTextureIndex(2);
        // man.setNormalIndex(2);
        // man.setSpecularIndex(2);
        // man.model = preLoadedModels["CesiumMan"];
        // // man.transform.translation = {0.5f, 0.5f, 0.0f};
        // // man.transform.scale = {3.0f, 1.0f, 3.0f};
        // // man.color = {128.0f, 228.1f, 229.1f}; //cyan
        // man.setTitle("CesiumMan");
        // gameObjects.emplace(man.getId(),std::move(man));

        // auto demon = VeGameObject::createGameObject();
        // demon.setTextureIndex(2);
        // demon.setNormalIndex(2);
        // demon.setSpecularIndex(2);
        // demon.model = preLoadedModels["Cute_Demon"];
        // // demon.transform.translation = {0.5f, 0.5f, 0.0f};
        // // demon.transform.scale = {3.0f, 1.0f, 3.0f};
        // // demon.color = {128.0f, 228.1f, 229.1f}; //cyan
        // demon.setTitle("CuteDemon");
        // gameObjects.emplace(demon.getId(),std::move(demon));

        //vase
//        auto cube = VeGameObject::createGameObject();
//        cube.setTextureIndex(1);
//        cube.setNormalIndex(1);
//        cube.setSpecularIndex(1);
//        cube.model = preLoadedModels["cube"];
//        cube.transform.translation = {1.5f, 0.5f, 0.0f};
//        cube.transform.scale = {0.45f, 0.45f, 0.45f};
//        cube.color = {128.0f, 228.1f, 229.1f}; //cyan
//        cube.setTitle("Cube");
//        gameObjects.emplace(cube.getId(),std::move(cube));
        // object 2: floor
//        auto quad = VeGameObject::createGameObject();
//        quad.setTextureIndex(2);
//        quad.setNormalIndex(2);
//        quad.setSpecularIndex(2);
//        quad.model = preLoadedModels["quad"];
//        quad.transform.translation = {0.0f, 0.5f, 0.0f};
//        quad.transform.scale = {3.0f, 0.5f, 3.0f};
//        quad.color = {103.0f,242.0f,209.0f};//light green
//        quad.setTitle("Floor");
//        gameObjects.emplace(quad.getId(),std::move(quad));

        //object 3: light
        auto light = VeGameObject::createPointLight(1.0f, .2f, {1.0f,1.0f,1.0f});
        light.setTitle("Light");
        light.transform.translation = {-0.811988f, -6.00838f, 0.1497f};
        light.lightComponent->lightIntensity = 20.0f;
        gameObjects.emplace(light.getId(),std::move(light));

        //skybox
        auto skybox = VeGameObject::createCubeMap(*veDevice, assetManager.get(), {"cubemap/right_white.png", "cubemap/left_white.png", "cubemap/top_white.png", "cubemap/bottom_white.png", "cubemap/front_white.png", "cubemap/back_white.png"}, *globalPool);
        skybox.setTitle("Skybox");
        gameObjects.emplace(skybox.getId(),std::move(skybox));
        engineInfo.cubeMapIndex = skybox.getId();
        LOGI("Successfully loaded game objects");
    }
    void FirstApp::loadTextures(){
        textures.push_back(std::make_unique<VeTexture>(*veDevice, assetManager.get(), "textures/brick_texture.png"));
        textures.push_back(std::make_unique<VeTexture>(*veDevice, assetManager.get(),"textures/metal.tga"));
        textures.push_back(std::make_unique<VeTexture>(*veDevice, assetManager.get(),"textures/wood.png"));
        // textures.push_back(std::make_unique<VeTexture>(*veDevice, assetManager.get(), "textures/wall_gray.png"));
        // textures.push_back(std::make_unique<VeTexture>(*veDevice, assetManager.get(), "textures/tile.png"));
        // textures.push_back(std::make_unique<VeTexture>(*veDevice, assetManager.get(), "textures/stone.png"));
        //normal maps
        normalMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/brick_normal.png", assetManager.get()));
        normalMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/metal_normal.tga", assetManager.get()));
        normalMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/wood_normal.png", assetManager.get()));
        // normalMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/wall_gray_normal.png", assetManager.get()));
        // normalMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/tile_normal.png", assetManager.get()));
        // normalMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/stone_normal.png", assetManager.get()));
        //specular maps
        specularMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/brick_specular.png", assetManager.get()));
        specularMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/metal_specular.tga", assetManager.get()));
        specularMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/wood_specular.png", assetManager.get()));
        // specularMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/wall_gray_specular.png", assetManager.get()));
        // specularMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/tile_specular.png", assetManager.get()));
        // specularMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/stone_specular.png", assetManager.get()));
        //get image infos
        for(int i = 0; i < textures.size(); i++){
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = textures[i]->getLayout();
            imageInfo.imageView = textures[i]->getImageView();
            imageInfo.sampler = textures[i]->getSampler();
            textureInfos.push_back(VkDescriptorImageInfo(imageInfo));
            VkDescriptorImageInfo normalImageInfo{};
            normalImageInfo.imageLayout = normalMaps[i]->getLayout();
            normalImageInfo.imageView = normalMaps[i]->getNormalImageView();
            normalImageInfo.sampler = normalMaps[i]->getNormalSampler();
            normalMapInfos.push_back(VkDescriptorImageInfo(normalImageInfo));
            VkDescriptorImageInfo specularImageInfo{};
            specularImageInfo.imageLayout = specularMaps[i]->getLayout();
            specularImageInfo.imageView = specularMaps[i]->getNormalImageView();
            specularImageInfo.sampler = specularMaps[i]->getNormalSampler();
            specularMapInfos.push_back(VkDescriptorImageInfo(specularImageInfo));
        }
        LOGI("Successfully loaded game textures");
    }
    int FirstApp::getNumLights(){
        int numLights = 0;
        for(auto& [key, object] : gameObjects){
            if(object.lightComponent!=nullptr){
                numLights++;
            }
        }
        return numLights;
    }

}