#pragma once
//project components
#include "ve_model.hpp"
#include "ve_game_object.hpp"
//core libraries
#include <imgui.h>
#include <android/asset_manager.h>
//std imports
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
namespace ve{
    template<typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest){
        std::hash<T> hasher;
        seed ^=std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    }
    inline std::vector<uint8_t> loadBinaryFileToVector(const char *file_path,
                                                       AAssetManager *assetManager) {
        std::vector<uint8_t> file_content;
        assert(assetManager);
        AAsset *file =
                AAssetManager_open(assetManager, file_path, AASSET_MODE_BUFFER);
        size_t file_length = AAsset_getLength(file);

        file_content.resize(file_length);

        AAsset_read(file, file_content.data(), file_length);
        AAsset_close(file);
        return file_content;
    };

    inline const std::vector<std::string> modelFileNames = {
        "Cute_Demon",
        "CesiumMan",
        "Fox"
    };
    inline const std::vector<std::string> textureFileNames = {
        "brick_texture",
        "metal",
        "wood",
        "wall_gray",
        "tile"
    };
    inline std::unordered_map<std::string, std::shared_ptr<VeModel>> preLoadedModels;
    inline void preLoadModels(VeDevice& veDevice, AAssetManager *assetManager){
//        preLoadedModels["cube"] = VeModel::createModelFromFile(veDevice, assetManager,"models/cube.obj");
//        preLoadedModels["quad"] = VeModel::createModelFromFile(veDevice, assetManager, "models/quad.obj");
//        preLoadedModels["flat_vase"] = VeModel::createModelFromFile(veDevice, assetManager, "models/flat_vase.obj");
//        preLoadedModels["smooth_vase"] = VeModel::createModelFromFile(veDevice, assetManager, "models/smooth_vase.obj");
//        preLoadedModels["colored_cube"] = VeModel::createModelFromFile(veDevice, assetManager, "models/colored_cube.obj");
        preLoadedModels["Cute_Demon"] = VeModel::createModelFromFile(veDevice, assetManager, "models/result.gltf");
        preLoadedModels["CesiumMan"] = VeModel::createModelFromFile(veDevice, assetManager, "models/CesiumManAnimations.gltf");
        preLoadedModels["Fox"] = VeModel::createModelFromFile(veDevice, assetManager, "models/scene.gltf");
    }
    inline void cleanupPreloadedModels() {
        for (auto& [key, model] : preLoadedModels) {
            model.reset();
        }
        preLoadedModels.clear();
    }
    inline void renderGameObjectDetails(VeGameObject& gameObject, bool& isHighlight){
        ImGui::SetNextWindowSize(ImVec2(650, 600), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);
        ImGui::Begin("Game Object Inspector",  nullptr,   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Checkbox("Highlight joints", &isHighlight);

        // Animation Section
        if (ImGui::CollapsingHeader("Animation")) {
            // Animation State
            ImGui::Text("Current Animation: %s",
                        gameObject.model->animationManager->getName().c_str()
            );

            std::string currentAnim = gameObject.model->animationManager->getName();
            float animProgress = gameObject.model->animationManager->getProgress(currentAnim);

            float animationDuration = gameObject.model->animationManager->getDuration(currentAnim);

            // Slider for animation scrubbing
            ImGui::Text("Animation: %s", currentAnim.c_str());

            // Animation progress slider
            bool timeChanged = ImGui::SliderFloat("Animation Time",
                                                  &gameObject.model->animationManager->currentAnimation->currentKeyFrameTime,
                                                  0.0f,
                                                  animationDuration,
                                                  "%.2f sec"
            );
            static float playbackSpeed = 1.0f;
            ImGui::SliderFloat("Playback Speed", &gameObject.model->animationManager->currentAnimation->playbackSpeed, 0.1f, 3.0f, "%.1fx");
            // Playback controls
            ImGui::Columns(3, "AnimControls", false);
            float& currentTime = gameObject.model->animationManager->currentAnimation->currentKeyFrameTime;
            // Rewind button
            if (ImGui::Button("Rewind", ImVec2(-1, 0))) {
                currentTime = fmax(0.0f,currentTime-1.2f);
            }
            ImGui::NextColumn();

            // Pause/Play toggle
            static bool isPaused = false;
            if (ImGui::Button(isPaused ? "Play" : "Pause", ImVec2(-1, 0))) {
                isPaused = !isPaused;
                if(isPaused)
                    gameObject.model->animationManager->stop();
                else
                    gameObject.model->animationManager->start(currentAnim);
            }
            ImGui::NextColumn();

            // Fast Forward button
            if (ImGui::Button("Fast Forward", ImVec2(-1, 0))) {
                currentTime += 1.2f;
            }
            ImGui::Columns(1);
            bool isRepeat = gameObject.model->animationManager->isCurrentAnimRepeat();
            if (ImGui::Checkbox("Repeat Animation", &isRepeat)) {
                gameObject.model->animationManager->setRepeat();
            }
            // Progress bar for visual reference
            ImGui::ProgressBar(animProgress, ImVec2(0.0f, 0.0f),
                               (std::to_string(int(animProgress * 100)) + "%").c_str()
            );
            // Available Animations Dropdown
            static int currentAnimIndex = 0;
            std::vector<std::string>animationNames = gameObject.model->animationManager->getAllNames();
            if (ImGui::BeginCombo("Select Animation",
                                  gameObject.model->animationManager->getName().c_str()))
            {
                for (int n = 0; n < gameObject.model->animationManager->getNumAvailableAnimations(); n++) {
                    bool isSelected = (currentAnimIndex == n);
                    if (ImGui::Selectable(animationNames[n].c_str(), isSelected)) {
                        currentAnimIndex = n;
                        gameObject.model->animationManager->start(animationNames[n]);
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        // Additional Sections can be added here (e.g., Rendering, Physics, etc.)
        if(ImGui::Button("Select Model")){
            ImGui::OpenPopup("Model Selection##replace");
        }
        if(ImGui::BeginPopup("Model Selection##replace")){
            ImGui::Text("Select a model: ");
            for(const auto& model: modelFileNames){
                if(ImGui::Selectable(model.c_str())){
                    gameObject.model->animationManager->stop();
                    gameObject.model = preLoadedModels[model];
                    gameObject.model->animationManager->start();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

}