#include "first_app.hpp"
#include "ve_imgui.hpp"
#include "utility.hpp"
#include "debug.hpp"

#include "ImGuizmo.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <array>
#include <stdexcept>
#include <cassert>
namespace ve {
    //cleanup
    FirstApp::~FirstApp() {
        cleanup();
    }

    void FirstApp::cleanup() {
        if (!engineInfo.engineInitialized) {
            return; // Already cleaned up
        }
        // Wait for device to be idle before cleanup
        if (veDevice && veDevice->device() != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(veDevice->device());
        }

        // Clean up ImGui
        VeImGui::cleanUpImGui();
        // Clean up render passes
        if (imGuiRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(veDevice->device(), imGuiRenderPass, nullptr);
            imGuiRenderPass = VK_NULL_HANDLE;
        }
        // Clean up descriptor pools
        if (imGuiPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(veDevice->device(), imGuiPool, nullptr);
            imGuiPool = VK_NULL_HANDLE;
        }
        // Clean up models
        if (g_modelManager) {
            g_modelManager->clearAll();
            g_modelManager.reset();
        }
        // Clean up game objects
        gameObjects.clear();

        // Clean up render systems
        pbrRenderSystem.reset();
        pointLightSystem.reset();
        outlineHighlightSystem.reset();
        cubeMapRenderSystem.reset();
        skeletonSystem.reset();


        // Clean up renderer (this should clean up its owned swapchain)
        if (veRenderer) {
            veRenderer.reset(); // Renderer's destructor should handle swapchain cleanup
        }

        // Clean up device
        if (veDevice) {
            veDevice.reset();
        }

        // Clean up window
        if (veWindow) {
            veWindow.reset();
        }

        engineInfo.engineInitialized = false;
    }

    void FirstApp::cleanupSurface() {
        if (veDevice && veDevice->device() != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(veDevice->device());
        }
        if (veRenderer) {
            // This should clean up the old swapchain but keep the renderer alive
            veRenderer->resetWindow(); // You might need to add this method
        }
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
//        preLoadModels(*veDevice, assetManager.get());
        g_modelManager = std::make_unique<ModelManager>(*veDevice, assetManager.get());
        g_modelManager->initializeModels(*veDevice, assetManager.get());
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
        shadowManager = std::make_unique<ShadowManager>(*veDevice);
        shadowManager->initialize();
        shadowSetLayout =  VeDescriptorSetLayout::Builder(*veDevice)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
                .build();
        for(int i=0; i<2; i++){
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = shadowManager->getLayout();
            imageInfo.imageView = shadowManager->getShadowCubeViews(i);
            imageInfo.sampler = shadowManager->getShadowSamplers(i);
            shadowMapInfos.push_back(imageInfo);
        }
        VeDescriptorWriter(*shadowSetLayout, *globalPool)
                .writeImage(0, shadowMapInfos.data(),2)
                .build(shadowDescriptorSet);
        shadowRenderSystem = std::make_unique<ShadowRenderSystem>(*veDevice, assetManager.get(),
                                                                  shadowManager->getShadowRenderPass(0),
                                                                  std::vector<VkDescriptorSetLayout>{gameObjects.at(engineInfo.animatedObjIndex).animationComponent->animationSetLayout->getDescriptorSetLayout()});
        pbrRenderSystem = std::make_unique<PbrRenderSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(),
            std::vector<VkDescriptorSetLayout>{globalSetLayout->getDescriptorSetLayout(), /*textureSetLayout->getDescriptorSetLayout(),*/
                               gameObjects.at(engineInfo.animatedObjIndex).animationComponent->animationSetLayout->getDescriptorSetLayout(),
                               gameObjects.at(engineInfo.selectedObject).model->materialComponent->materialSetLayout->getDescriptorSetLayout(),
                               shadowSetLayout->getDescriptorSetLayout()});
        pointLightSystem = std::make_unique<PointLightSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout());
        outlineHighlightSystem = std::make_unique<OutlineHighlightSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout());
//        cubeMapRenderSystem = std::make_unique<CubeMapRenderSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(),
//            std::vector<VkDescriptorSetLayout>{globalSetLayout->getDescriptorSetLayout(), gameObjects.at(engineInfo.cubeMapIndex).cubeMapComponent->descriptorSetLayout->getDescriptorSetLayout()});
        skeletonSystem = std::make_unique<SkeletonSystem>(*veDevice, assetManager.get(), veRenderer->getSwapChainRenderPass(), std::vector<VkDescriptorSetLayout>{globalSetLayout->getDescriptorSetLayout()}, veRenderer->getAspectRatio());
        //imgui
        imGuiRenderPass = VeImGui::createRenderPass(veDevice->device(), veRenderer->getSwapChainImageFormat(), veRenderer->getSwapChainDepthFormat());
        VeImGui::createImGuiContext(*veDevice, *veWindow, imGuiPool, imGuiRenderPass, VeSwapChain::MAX_FRAMES_IN_FLIGHT);
        gameObjects.at(engineInfo.animatedObjIndex).model->animationManager->start(5);
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

            //record frame data
            engineInfo.numLights = getNumLights(); //multiple point lights
            engineInfo.frameIndex = veRenderer->getFrameIndex();
            FrameInfo frameInfo{engineInfo.frameIndex, engineInfo.frameTime, engineInfo.elapsedTime, commandBuffer, engineInfo.camera, globalDescriptorSets[engineInfo.frameIndex], gameObjects.at(engineInfo.animatedObjIndex).animationComponent->animationDescriptorSets[engineInfo.frameIndex], gameObjects, engineInfo.animatedObjIndex, engineInfo.selectedJointIndex,  engineInfo.numLights, engineInfo.showOutlignHighlight};
            //update global UBO
            GlobalUbo globalUbo{};
            globalUbo.projection = engineInfo.camera.getProjectionMatrix();
            globalUbo.view = engineInfo.camera.getRotViewMatrix();
            globalUbo.inverseView = engineInfo.camera.getInverseMatrix();
            globalUbo.selectedLight = engineInfo.selectedObject;
            globalUbo.frameTime = engineInfo.frameTime;
            pointLightSystem->update(frameInfo, globalUbo);
            uniformBuffers[engineInfo.frameIndex]->writeToBuffer(&globalUbo);
            uniformBuffers[engineInfo.frameIndex]->flush();
            //update animation
            gameObjects.at(engineInfo.animatedObjIndex).updateAnimation(engineInfo.frameTime, engineInfo.frameCount, engineInfo.frameIndex);
            std::vector<PointLight> pointLightsVec(std::begin(globalUbo.pointLights), std::end(globalUbo.pointLights));

            if(engineInfo.frameCount%2==0){
                shadowManager->renderShadowMaps(
                        commandBuffer,
                        pointLightsVec,
                        shadowRenderSystem->getPipelineLayout(),
                        shadowRenderSystem->getPipeline(),
                        [this, &frameInfo, &pointLightsVec](VkCommandBuffer cmd, int lightIndex,
                                                            int faceIndex,
                                                            const glm::mat4 &viewProjMatrix) {
                            // Use the new synchronized method
                            this->renderShadowFace(cmd, lightIndex, faceIndex, viewProjMatrix,
                                                   frameInfo, pointLightsVec.data());
                        }
                );
            }

            //render scene
            veRenderer->beginSwapChainRenderPass(commandBuffer);

            if(engineInfo.showOutlignHighlight) {
//                outlineHighlightSystem->renderGameObjects(frameInfo);
                pbrRenderSystem->renderGameObjects(frameInfo, /*shadowRenderSystem.getShadowDescriptorSet(frameIndex),*/ {globalDescriptorSets[engineInfo.frameIndex], /*textureDescriptorSet,*/ gameObjects.at(engineInfo.animatedObjIndex).animationComponent->animationDescriptorSets[engineInfo.frameIndex], gameObjects.at(engineInfo.selectedObject).model->materialComponent->materialDescriptorSets, shadowDescriptorSet});
                skeletonSystem->renderJoints(frameInfo, {globalDescriptorSets[engineInfo.frameIndex]});
                skeletonSystem->renderJointConnections(frameInfo, {globalDescriptorSets[engineInfo.frameIndex]});
            }else
                pbrRenderSystem->renderGameObjects(frameInfo, /*shadowRenderSystem.getShadowDescriptorSet(frameIndex),*/ {globalDescriptorSets[engineInfo.frameIndex], /*textureDescriptorSet,*/ gameObjects.at(engineInfo.animatedObjIndex).animationComponent->animationDescriptorSets[engineInfo.frameIndex], gameObjects.at(engineInfo.selectedObject).model->materialComponent->materialDescriptorSets, shadowDescriptorSet});
            pointLightSystem->render(frameInfo);
//            cubeMapRenderSystem->renderGameObjects(frameInfo);

            //start new imgui frame
            VeImGui::initializeImGuiFrame();
            //imgui windows
            updateModelLoadingStatus(); //show loading spinner when loading a model
            if(engineInfo.showModelsList) //show dogs list when choosing a model
                renderDogModelList();
            if(engineInfo.changeAnimationList)
                renderChangeAnimationList();
            if(engineInfo.showEditJointMode) { //show joint editor when editing joints
                animationSequencer.BeginFrame();
                animationSequencer.ShowSequencerWindow(gameObjects.at(engineInfo.animatedObjIndex), engineInfo);
                animationSequencer.RenderJointGizmo(gameObjects.at(engineInfo.animatedObjIndex), engineInfo);
            }

            VeImGui::renderImGuiFrame(commandBuffer);
            veRenderer->endSwapChainRenderPass(commandBuffer);
            veRenderer->endFrame();
        }
        engineInfo.frameCount++;
//        LOGI("Rendering frame: %d", engineInfo.frameCount);
    }
    void FirstApp::setModelMode(bool value) {
        inputHandler.setModelMode(value);
    }
    void FirstApp::controlCamera(){
        if(inputHandler.getModelMode()) {
            inputHandler.processModelMovement(gameObjects.at(engineInfo.selectedObject).transform);
        }else {
            inputHandler.processMovement(engineInfo.camera);
        }
    }

    void FirstApp::loadGameObjects() {
        auto model = g_modelManager->getModel("Akita Inu");
        auto fox = VeGameObject::createAnimatedObject(*veDevice, *globalPool, model);
//        auto fox = VeGameObject::createGameObject();
        fox.setTextureIndex(1);
//        fox.model = preLoadedModels["Cute_Demon"];
        // fox.transform.translation = {0.5f, 0.5f, 0.0f};
        fox.transform.scale = {5.0f, 5.0f, 5.0f};
        // fox.color = {128.0f, 228.1f, 229.1f}; //cyan
        fox.setTitle("Akita Inu");
        gameObjects.emplace(fox.getId(),std::move(fox));
        engineInfo.animatedObjIndex = fox.getId();

        auto quad = VeGameObject::createGameObject();
        quad.model = VeModel::createQuad(*veDevice);
        quad.setTextureIndex(0);
        quad.transform.scale = {100.0f, 100.0f, 100.0f};
        quad.color = {128.0f, 228.1f, 229.1f}; //cyan
        gameObjects.emplace(quad.getId(),std::move(quad));

        //object 3: light
        auto light = VeGameObject::createPointLight(1.0f, .2f, {1.0f,1.0f,1.0f});
        light.setTitle("Light");
        light.transform.translation = {4.0f, 6.00838f, 0.0f};
        light.lightComponent->lightIntensity = 30.0f;
        gameObjects.emplace(light.getId(),std::move(light));

        auto light3 = VeGameObject::createPointLight(1.0f, .2f, {1.0f,1.0f,1.0f});
        light3.setTitle("Light2");
        light3.transform.translation = {-4.0f, 6.00838f, 0};
        light3.lightComponent->lightIntensity = 30.0f;
        gameObjects.emplace(light3.getId(),std::move(light3));
        //skybox
//        auto skybox = VeGameObject::createCubeMap(*veDevice, assetManager.get(), {"cubemap/right_white.png", "cubemap/left_white.png", "cubemap/top_white.png", "cubemap/bottom_white.png", "cubemap/front_white.png", "cubemap/back_white.png"}, *globalPool);
//        skybox.setTitle("Skybox");
//        gameObjects.emplace(skybox.getId(),std::move(skybox));
//        engineInfo.cubeMapIndex = skybox.getId();

        LOGI("Successfully loaded game objects");
    }
    void FirstApp::loadTextures(){
        textures.push_back(std::make_unique<VeTexture>(*veDevice, assetManager.get(),"textures/stone.png"));
        textures.push_back(std::make_unique<VeTexture>(*veDevice, assetManager.get(),"textures/tile.png"));
        textures.push_back(std::make_unique<VeTexture>(*veDevice, assetManager.get(),"textures/wood.png"));
        //normal maps
        normalMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/stone_normal.png", assetManager.get()));
        normalMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/tile_normal.png", assetManager.get()));
        normalMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/wood_normal.png", assetManager.get()));
        //specular maps
        specularMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/stone_specular.png", assetManager.get()));
        specularMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/tile_specular.png", assetManager.get()));
        specularMaps.push_back(std::make_unique<VeNormal>(*veDevice, "textures/wood_specular.png", assetManager.get()));

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
    std::string FirstApp::changeModel(std::string name){
        // Check if the model is already loaded and ready
        if (g_modelManager->isModelReady(name)) {
            gameObjects.at(engineInfo.selectedObject).model = g_modelManager->getModel(name);
            gameObjects.at(engineInfo.selectedObject).model->animationManager->start(engineInfo.currentAnimationIndex);
            engineInfo.currentLoadingModelName = ""; // Clear any previous loading state
            return name + " loaded successfully";
        }
        // If not ready, check if it's currently loading
        if (g_modelManager->isModelLoading(name)) {
            engineInfo.currentLoadingModelName = name; // Indicate that this model is being loaded
            return name + " is currently loading...";
        }
        // If neither loaded nor loading, initiate the load
        // The getModel call will internally start async loading if not in cache
        std::shared_ptr<VeModel> model = g_modelManager->getModel(name);

        if (model) { // This means it was either immediately available or became available
            // right after getModel() was called (less common for large models)
            gameObjects.at(engineInfo.selectedObject).model = model;
            gameObjects.at(engineInfo.selectedObject).model->animationManager->start(engineInfo.currentAnimationIndex);
            engineInfo.currentLoadingModelName = "";
            return name + " loaded immediately";
        } else {
            // Model is not yet ready, loading has been initiated or is in progress
            engineInfo.currentLoadingModelName = name; // Set this to trigger UI loading indicator
            return name + " started loading...";
        }
    }
    void FirstApp::updateModelLoadingStatus() {
        if (!engineInfo.currentLoadingModelName.empty()) {
            drawSimpleSpinner("Loading " + engineInfo.currentLoadingModelName + "...");
            auto model = g_modelManager->getModel(engineInfo.currentLoadingModelName);
            if (g_modelManager->isModelReady(engineInfo.currentLoadingModelName)) {
                gameObjects.at(engineInfo.selectedObject).model = g_modelManager->getModel(engineInfo.currentLoadingModelName);
                gameObjects.at(engineInfo.selectedObject).model->animationManager->start(engineInfo.currentAnimationIndex);
                std::string loadedModelName = engineInfo.currentLoadingModelName;
                engineInfo.currentLoadingModelName = "";
            } else if (!g_modelManager->isModelLoading(engineInfo.currentLoadingModelName)) {

                std::string failedModelName = engineInfo.currentLoadingModelName;
                engineInfo.currentLoadingModelName = "";
            }
        }

    }
    void FirstApp::drawSimpleSpinner(const std::string& loadingText) {
        // Get the main viewport
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 center = viewport->GetCenter();

        // Set next window position to center
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        // Create overlay window
        ImGui::SetNextWindowBgAlpha(0.8f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoScrollbar |
                                 ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("Loading", nullptr, flags)) {
            // Simple text-based spinner
            static const char* spinner_chars = "|/-\\";
            static int spinner_index = 0;
            static float last_time = 0.0f;

            float current_time = ImGui::GetTime();
            if (current_time - last_time > 0.1f) { // Update every 100ms
                spinner_index = (spinner_index + 1) % 4;
                last_time = current_time;
            }

            ImGui::Text("%c Loading...", spinner_chars[spinner_index]);

            if (!loadingText.empty()) {
                ImGui::Text("%s", loadingText.c_str());
            }
        }
        ImGui::End();
    }

    void FirstApp::renderDogModelList() {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(50.0, io.DisplaySize.y*0.5-300.0));
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);
        ImGui::Begin("Select a model",  nullptr,   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        auto &obj = gameObjects.at(engineInfo.selectedObject);
        for(const auto& model: g_modelManager->getAvailableModels()){
            if(ImGui::Selectable(model.c_str())){
                //stop animation before changing model
                if(gameObjects.at(engineInfo.selectedObject).model->hasAnimationData())
                    obj.model->animationManager->stop();
                if(g_modelManager->isModelReady(model)){
                    obj.model = g_modelManager->getModel(model);
                    if(obj.model->hasAnimationData()) {
                        //start animation for new model
                        obj.model->animationManager->start(engineInfo.currentAnimationIndex);
                        obj.setTextureIndex(1); //temporary fix
                        obj.transform.scale = glm::vec3(5.0f);
                        obj.transform.translation.y =0.0f;
                    }
                }else if(g_modelManager->isModelLoading(model)){
                    engineInfo.currentLoadingModelName = model;
                }else{
                    auto modelInstance = g_modelManager->getModel(model);
                    if (modelInstance) { // This means it was either immediately available or became available
                        // right after getModel() was called (less common for large models)
                        obj.model = modelInstance;
                        engineInfo.currentLoadingModelName = "";
                        if(obj.model->hasAnimationData()) {
                            //start animation for new model
                            obj.model->animationManager->start(engineInfo.currentAnimationIndex);
                            obj.setTextureIndex(1); //temporary fix
                            obj.transform.scale = glm::vec3(5.0f);
                            obj.transform.translation.y =0.0f;
                        }
                    } else {
                        // Model is not yet ready, loading has been initiated or is in progress
                        engineInfo.currentLoadingModelName = model; // Set this to trigger UI loading indicator
                    }
                }

                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
    void FirstApp::renderChangeAnimationList(){
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(50.0, io.DisplaySize.y*0.5-300.0));
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);
        ImGui::Begin("Select an animation",  nullptr,   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        auto &obj = gameObjects.at(engineInfo.selectedObject);std::vector<std::string>animationNames = obj.model->animationManager->getAllNames();
        for (int n = 0; n < obj.model->animationManager->getNumAvailableAnimations(); n++) {
            bool isSelected = (engineInfo.currentAnimationIndex == n);
            if (ImGui::Selectable(animationNames[n].c_str(), isSelected)) {
                engineInfo.currentAnimationIndex = n;
                obj.model->animationManager->start(animationNames[n]);
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
    bool FirstApp::toggleAnimationPlayer(bool value) {
        auto &obj = gameObjects.at(engineInfo.selectedObject);

        // No animation data: always "paused"
        if (!obj.model->hasAnimationData() || obj.model->animationManager->currentAnimation == nullptr) {
            return false; // false = paused
        }

        if (value) {
            obj.model->animationManager->start(engineInfo.currentAnimationIndex);
            return true;  // true = playing
        } else {
            obj.model->animationManager->stop();
            return false; // false = paused
        }
    }
    void FirstApp::jumpForward() {
        auto &obj = gameObjects.at(engineInfo.selectedObject);
        float& currentTime = obj.model->animationManager->currentAnimation->currentKeyFrameTime;
        currentTime += 0.02f;
        obj.model->animationManager->updateTimeSkip(*obj.model->skeleton);
    }
    void FirstApp::jumpBackward() {
        auto &obj = gameObjects.at(engineInfo.selectedObject);
        float& currentTime = obj.model->animationManager->currentAnimation->currentKeyFrameTime;
        currentTime = fmax(0.0f,currentTime-0.02f);
        obj.model->animationManager->updateTimeSkip(*obj.model->skeleton);
    }
    float FirstApp::getAnimationDuration() {
        if (gameObjects.empty() || engineInfo.selectedObject >= gameObjects.size()) {
            return 0.0f;
        }

        auto &obj = gameObjects.at(engineInfo.selectedObject);
        if (!obj.model || !obj.model->hasAnimationData() || obj.model->animationManager->currentAnimation == nullptr) {
            return 0.0f;
        }

        std::string currentAnim = obj.model->animationManager->getName();
        float animationDuration = obj.model->animationManager->getDuration(currentAnim);
        return animationDuration;
    }
    void FirstApp::setAnimationTime(float time) {
        if (gameObjects.empty() || engineInfo.selectedObject >= gameObjects.size()) {
            return;
        }

        auto &obj = gameObjects.at(engineInfo.selectedObject);
        if (!obj.model || !obj.model->hasAnimationData() || obj.model->animationManager->currentAnimation == nullptr) {
            return;
        }
        float duration = getAnimationDuration();
        time = std::max(0.0f, std::min(time, duration));

        obj.model->animationManager->currentAnimation->currentKeyFrameTime = time;
        obj.model->animationManager->stop();

        // Force update animation to the new time immediately
        obj.model->animationManager->updateTimeSkip(*obj.model->skeleton);
    }
    float FirstApp::getCurrentAnimationTime() {
        if (gameObjects.empty() || engineInfo.selectedObject >= gameObjects.size()) {
            return 0.0f;
        }

        auto &obj = gameObjects.at(engineInfo.selectedObject);
        if (!obj.model || !obj.model->hasAnimationData()) {
            return 0.0f;
        }
        if(obj.model->animationManager->currentAnimation == nullptr)
            return 0.0f;
        return obj.model->animationManager->currentAnimation->currentKeyFrameTime;
    }
    void FirstApp::pauseAnimation() {
        if (gameObjects.empty() || engineInfo.selectedObject >= gameObjects.size()) {
            return;
        }

        auto &obj = gameObjects.at(engineInfo.selectedObject);
        if (!obj.model || !obj.model->animationManager || obj.model->animationManager->currentAnimation==nullptr) {
            return;
        }

        obj.model->animationManager->stop();
    }

    void FirstApp::resumeAnimation() {
        if (gameObjects.empty() || engineInfo.selectedObject >= gameObjects.size()) {
            return;
        }

        auto &obj = gameObjects.at(engineInfo.selectedObject);
        if (!obj.model || !obj.model->animationManager || obj.model->animationManager->currentAnimation==nullptr) {
            return;
        }

        obj.model->animationManager->start(engineInfo.currentAnimationIndex);
    }
    bool FirstApp::isAnimationPlaying() {
        if (gameObjects.empty() || engineInfo.selectedObject >= gameObjects.size()) {
            return false;
        }

        auto &obj = gameObjects.at(engineInfo.selectedObject);
        if (!obj.model || !obj.model->hasAnimationData() || obj.model->animationManager->currentAnimation==nullptr) {
            return false;
        }

        return obj.model->animationManager->currentAnimation->isRunning();
    }
    void FirstApp::renderShadowFace(VkCommandBuffer commandBuffer, int lightIndex, int faceIndex, const glm::mat4& viewProjMatrix, FrameInfo frameInfo, PointLight lights[10]) {
        // Get the light data
        const auto& light = lights[lightIndex];
        // Use the new synchronized method
        shadowRenderSystem->renderObjectsForShadowFace(
                commandBuffer,
                frameInfo,
                lightIndex,
                faceIndex,
                viewProjMatrix,
                light.position,
                light.radius
        );
    }
}