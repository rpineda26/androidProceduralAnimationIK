#pragma once
//project components
#include "ve_model.hpp"
#include "ve_game_object.hpp"
#include "debug.hpp"
//core libraries
#include "imgui_internal.h"
#include <imgui.h>
#include <android/asset_manager.h>
#include <glm/gtx/matrix_decompose.hpp>
//std imports
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <limits>
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
        "tile",
        "fox_material_basColor"

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
    inline bool ZoomableTimelineWidget(const char* label, float* currentTime, float duration, const std::vector<float>& keyframes, float viewDuration = 2.0f, int maxVisibleDiamonds = 10) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImGuiID storageId = window->GetID((label + std::string("_view")).c_str());
        ImGuiStorage* storage = ImGui::GetStateStorage();

        // --- View Range Calculation & State Handling ---
        float viewStartTime = storage->GetFloat(storageId, 0.0f); // Get previous view start
        float viewEndTime = viewStartTime + viewDuration;

        // Check if currentTime is outside view or too close to edges (within 10% of the edge)
        float edgeBuffer = viewDuration * 0.1f;
        bool needViewUpdate =
                *currentTime < viewStartTime + edgeBuffer ||
                *currentTime > viewEndTime - edgeBuffer;

        if (needViewUpdate) {
            // Only recenter when needed
            viewStartTime = *currentTime - viewDuration / 2.0f;
        }

        // Apply boundary clamping after deciding whether to update
        viewStartTime = std::max(0.0f, viewStartTime);
        float currentViewEnd = viewStartTime + viewDuration;
        currentViewEnd = std::min(duration, currentViewEnd);
        float currentViewDuration = currentViewEnd - viewStartTime;
        viewStartTime = std::max(0.0f, currentViewEnd - currentViewDuration);

        // --- Widget Layout & Hitbox ---
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        float available_width = ImGui::GetContentRegionAvail().x;
        if (available_width <= 0.0f) available_width = 100.0f;
        float height = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f;
        ImVec2 size(available_width, height);
        ImRect bb(cursor_pos, ImVec2(cursor_pos.x + size.x, cursor_pos.y + size.y));
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id)) return false;

        // --- Interaction Handling (with Snapping) ---
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);
        bool value_changed = false;
        if (hovered && (held || pressed) && currentViewDuration > 0.0f) {
            ImVec2 mouse_pos_rel = ImVec2(g.IO.MousePos.x - bb.Min.x, g.IO.MousePos.y - bb.Min.y);
            float time_in_view = (ImClamp(mouse_pos_rel.x, 0.0f, size.x) / size.x) * currentViewDuration;
            float new_time = viewStartTime + time_in_view;
            new_time = ImClamp(new_time, 0.0f, duration);

            float snap_radius_pixels = height * 0.5f;
            float min_snap_diff = std::numeric_limits<float>::max();
            float snapped_time = new_time;
            bool did_snap = false;
            for (float key_time : keyframes) {
                if (key_time >= viewStartTime && key_time <= currentViewEnd) {
                    float key_x = bb.Min.x + ((key_time - viewStartTime) / currentViewDuration) * size.x;
                    float pixel_diff = std::fabs(g.IO.MousePos.x - key_x);
                    if (pixel_diff < snap_radius_pixels && pixel_diff < min_snap_diff) {
                        min_snap_diff = pixel_diff; snapped_time = key_time; did_snap = true;
                    }
                }
            }
            if (did_snap) { new_time = snapped_time; }

            if (*currentTime != new_time) {
                *currentTime = new_time; value_changed = true; ImGui::MarkItemEdited(id);
            }
        }

        // --- Drawing ---
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), style.FrameRounding);

        if (duration <= 0.0f || currentViewDuration <= 0.0f) {
            char time_str_empty[64]; ImFormatString(time_str_empty, sizeof(time_str_empty), "Time: %.2f / %.2f s", *currentTime, duration); ImGui::TextUnformatted(time_str_empty);
            storage->SetFloat(storageId, viewStartTime);
            return value_changed;
        }

        // Draw Base Timeline Bar
        draw_list->AddLine(ImVec2(bb.Min.x, bb.Min.y + size.y * 0.5f),
                           ImVec2(bb.Max.x, bb.Min.y + size.y * 0.5f),
                           ImGui::GetColorU32(ImGuiCol_SliderGrab));

        // Prepare Keyframes for Drawing (Filter & Decimate)
        std::vector<float> visibleKeyframes;
        visibleKeyframes.reserve(keyframes.size());
        for(float kt : keyframes) {
            if (kt >= viewStartTime && kt <= currentViewEnd) {
                visibleKeyframes.push_back(kt);
            }
        }

        // --- [IMPROVED] Highlight Logic - persist highlight until next keyframe is reached ---
        float activeKeyframeTime = -1.0f;

        // Find which keyframe's interval the current time is in
        for (size_t i = 0; i < visibleKeyframes.size(); ++i) {
            float current_key = visibleKeyframes[i];
            float next_key = (i + 1 < visibleKeyframes.size()) ? visibleKeyframes[i + 1] : duration;

            // If currentTime is in this interval [current_key, next_key)
            // Use a small epsilon for floating point comparison
            const float epsilon = 0.00001f;
            if (*currentTime >= current_key - epsilon &&
                (i + 1 == visibleKeyframes.size() || *currentTime < next_key - epsilon)) {
                activeKeyframeTime = current_key;
                break;
            }
        }

        std::vector<float> keyframesToDraw;
        if (visibleKeyframes.empty()) { /* no keys */ }
        else if (visibleKeyframes.size() <= static_cast<size_t>(maxVisibleDiamonds)) {
            keyframesToDraw = visibleKeyframes;
        }
        else { /* Decimation */
            keyframesToDraw.push_back(visibleKeyframes.front());
            float stride = std::max(1.0f, static_cast<float>(visibleKeyframes.size() - 2) / static_cast<float>(maxVisibleDiamonds - 2));
            float next_target_index = stride;
            for (size_t i = 1; i < visibleKeyframes.size() - 1; ++i) {
                if (static_cast<float>(i) >= next_target_index) {
                    if (keyframesToDraw.empty() || std::fabs(visibleKeyframes[i] - keyframesToDraw.back()) > 0.00001f) {
                        keyframesToDraw.push_back(visibleKeyframes[i]);
                    }
                    next_target_index += stride;
                }
            }
            if (visibleKeyframes.back() != keyframesToDraw.back()) {
                keyframesToDraw.push_back(visibleKeyframes.back());
            }
        }

        // --- Draw Keyframe Markers (Decimated) ---
        float diamond_half_size = height * 0.25f;
        ImU32 marker_color_default_outline = ImGui::GetColorU32(ImGuiCol_Button);
        ImU32 marker_color_highlight_fill = ImGui::GetColorU32(ImGuiCol_NavHighlight);
        float min_pixel_distance = diamond_half_size * 1.5f;
        float last_drawn_marker_x = -std::numeric_limits<float>::max();

        for (size_t i = 0; i < keyframesToDraw.size(); ++i) {
            float key_time = keyframesToDraw[i];

            float key_x = bb.Min.x + ((key_time - viewStartTime) / currentViewDuration) * size.x;
            float key_y = bb.Min.y + size.y * 0.5f;

            if (key_x >= last_drawn_marker_x + min_pixel_distance) {
                // Using our improved highlighting logic
                bool is_highlighted = (key_time == activeKeyframeTime);

                ImVec2 diamond_verts[4] = {
                        ImVec2(key_x, key_y - diamond_half_size), ImVec2(key_x + diamond_half_size, key_y),
                        ImVec2(key_x, key_y + diamond_half_size), ImVec2(key_x - diamond_half_size, key_y)
                };
                if (is_highlighted) {
                    draw_list->AddConvexPolyFilled(diamond_verts, 4, marker_color_highlight_fill);
                } else {
                    draw_list->AddPolyline(diamond_verts, 4, marker_color_default_outline, ImDrawFlags_Closed, 1.0f);
                }

                last_drawn_marker_x = key_x;
            }
        }

        // --- Tooltip Logic (Check against ALL VISIBLE keyframes) ---
        float tooltip_handle_radius = height * 0.4f;
        for(float key_time : visibleKeyframes) {
            float key_x = bb.Min.x + ((key_time - viewStartTime) / currentViewDuration) * size.x;
            float key_y = bb.Min.y + size.y * 0.5f;
            ImVec2 marker_center(key_x, key_y);
            if (ImGui::IsMouseHoveringRect(ImVec2(marker_center.x - tooltip_handle_radius, marker_center.y - tooltip_handle_radius),
                                           ImVec2(marker_center.x + tooltip_handle_radius, marker_center.y + tooltip_handle_radius))) {
                ImGui::SetTooltip("Keyframe: %.2f s", key_time);
            }
        }

        // --- Draw Playhead ---
        if (*currentTime >= viewStartTime && *currentTime <= currentViewEnd) {
            float current_x = bb.Min.x + ((*currentTime - viewStartTime) / currentViewDuration) * size.x;
            ImU32 playhead_color = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
            draw_list->AddLine(ImVec2(current_x, bb.Min.y), ImVec2(current_x, bb.Max.y), playhead_color, 1.5f);
            float triangle_half_width = height * 0.25f;
            float triangle_height = height * 0.3f;
            draw_list->AddTriangleFilled(
                    ImVec2(current_x, bb.Min.y + triangle_height),
                    ImVec2(current_x - triangle_half_width, bb.Min.y),
                    ImVec2(current_x + triangle_half_width, bb.Min.y),
                    playhead_color);
        }

        // --- Display Time Text ---
        char time_str[128];
        ImFormatString(time_str, sizeof(time_str), "Time: %.2f / %.2f s", *currentTime, duration);
        ImGui::TextUnformatted(time_str);

        // --- Store view state for next frame ---
        storage->SetFloat(storageId, viewStartTime);

        return value_changed;
    }
    inline bool TimelineWidget(const char* label, float* currentTime, float duration, const std::vector<float>& keyframes) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        float available_width = ImGui::GetContentRegionAvail().x;
        float height = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f; // Timeline height
        ImVec2 size(available_width, height);

        ImRect bb(cursor_pos, ImVec2(cursor_pos.x + size.x, cursor_pos.y + size.y));
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);
        bool value_changed = false;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // --- Background ---
        draw_list->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), style.FrameRounding);

        if (duration <= 0.0f) // Nothing to draw if duration is zero or negative
            return false;

        float handle_radius = height * 0.4f; // Radius for clickable area around markers/playhead

        // --- Interaction ---
        if (hovered && (held || pressed)) {
            ImVec2 mouse_pos_rel = ImVec2(g.IO.MousePos.x - bb.Min.x, g.IO.MousePos.y - bb.Min.y);
            float new_time = (mouse_pos_rel.x / size.x) * duration;
            new_time = ImClamp(new_time, 0.0f, duration); // Clamp to valid range

            // Check if clicking near a keyframe marker to snap
            bool snapped = false;
            for (float key_time : keyframes) {
                float key_x = (key_time / duration) * size.x;
                if (std::fabs(mouse_pos_rel.x - key_x) < handle_radius) { // Snap radius
                    new_time = key_time;
                    snapped = true;
                    break;
                }
            }

            if (*currentTime != new_time) {
                *currentTime = new_time;
                value_changed = true;
                ImGui::MarkItemEdited(id);
            }
        }

        // --- Draw Timeline Bar & Ticks (Optional) ---
        draw_list->AddLine(ImVec2(bb.Min.x, bb.Min.y + size.y * 0.5f),
                           ImVec2(bb.Max.x, bb.Min.y + size.y * 0.5f),
                           ImGui::GetColorU32(ImGuiCol_SliderGrab)); // Use a subtle color

        // --- Draw Keyframe Markers (Diamonds) ---
        float diamond_half_size = height * 0.25f;
        ImU32 marker_color = ImGui::GetColorU32(ImGuiCol_Button);
        float min_pixel_distance = diamond_half_size * 1.5f;
        float last_drawn_marker_x = -FLT_MAX;
        LOGI("drawing markers!");
        for (float key_time : keyframes) {
            float key_x = bb.Min.x + (key_time / duration) * size.x;
            float key_y = bb.Min.y + size.y * 0.5f;
            LOGI("KeyTime=%.2f -> KeyX=%.1f\n", key_time, key_x);
            if (key_x >= last_drawn_marker_x + min_pixel_distance)
            {
                // Draw diamond using 4 lines
                draw_list->AddLine(ImVec2(key_x, key_y - diamond_half_size), ImVec2(key_x + diamond_half_size, key_y), marker_color);
                draw_list->AddLine(ImVec2(key_x + diamond_half_size, key_y), ImVec2(key_x, key_y + diamond_half_size), marker_color);
                draw_list->AddLine(ImVec2(key_x, key_y + diamond_half_size), ImVec2(key_x - diamond_half_size, key_y), marker_color);
                draw_list->AddLine(ImVec2(key_x - diamond_half_size, key_y), ImVec2(key_x, key_y - diamond_half_size), marker_color);

                last_drawn_marker_x = key_x;
            }
            ImVec2 marker_center(key_x, key_y);
            if (ImGui::IsMouseHoveringRect(ImVec2(marker_center.x - handle_radius, marker_center.y - handle_radius),
                                           ImVec2(marker_center.x + handle_radius, marker_center.y + handle_radius)))
            {
                ImGui::SetTooltip("Keyframe: %.2f s", key_time);
            }
        }

        float current_x = bb.Min.x + (*currentTime / duration) * size.x;
        ImU32 playhead_color = ImGui::GetColorU32(ImGuiCol_ButtonHovered); // A distinct color
        float triangle_base_y = bb.Max.y - style.FramePadding.y; // Position triangle near bottom edge

        draw_list->AddLine(ImVec2(current_x, bb.Min.y), ImVec2(current_x, bb.Max.y), playhead_color, 1.5f);

        // Triangle Handle (pointing down, centered on the line)
        float triangle_half_width = height * 0.25f;
        float triangle_height = height * 0.3f;
        draw_list->AddTriangleFilled(
                ImVec2(current_x, bb.Min.y + triangle_height), // Top point
                ImVec2(current_x - triangle_half_width, bb.Min.y), // Bottom-left
                ImVec2(current_x + triangle_half_width, bb.Min.y), // Bottom-right
                playhead_color
        );

        // Display current time text centered below the timeline
        char time_str[64];
        ImFormatString(time_str, sizeof(time_str), "%.2f / %.2f s", *currentTime, duration);
        ImVec2 text_size = ImGui::CalcTextSize(time_str);

        ImGui::TextUnformatted(time_str);


        return value_changed;
    }

    inline void UpdateOrCreateKeyframe(ve::Animation::Sampler& sampler, float time, const glm::vec4& value) {
        // Look for existing keyframe at this time
        bool keyframeExists = false;
        size_t keyframeIndex = 0;

        for (size_t j = 0; j < sampler.timeStamps.size(); j++) {
            if (std::abs(sampler.timeStamps[j] - time) < 0.001f) {
                keyframeExists = true;
                keyframeIndex = j;
                break;
            }
        }

        // Update or add keyframe
        if (keyframeExists) {
            // Update existing keyframe
            sampler.TRSoutputValues[keyframeIndex] = value;
        } else {
            // Add new keyframe
            // Find insertion point to maintain sorted order
            size_t insertIndex = sampler.timeStamps.size();
            for (size_t j = 0; j < sampler.timeStamps.size(); j++) {
                if (sampler.timeStamps[j] > time) {
                    insertIndex = j;
                    break;
                }
            }

            // Insert the new keyframe
            sampler.timeStamps.insert(sampler.timeStamps.begin() + insertIndex, time);
            sampler.TRSoutputValues.insert(sampler.TRSoutputValues.begin() + insertIndex, value);
        }
    }

    inline void CreateAnimationChannel(ve::Animation& animation, ve::Skeleton* skeleton, int jointIndex,
                                float time, ve::Animation::PathType pathType, const glm::vec4& value) {
        // Create new sampler
        ve::Animation::Sampler newSampler;
        newSampler.timeStamps = {time};
        newSampler.TRSoutputValues = {value};
        newSampler.interpolationMethod = ve::Animation::InterpolationMethod::LINEAR;

        // Add sampler to animation
        size_t samplerIndex = animation.samplers.size();
        animation.samplers.push_back(newSampler);

        // Create new channel
        ve::Animation::Channel newChannel;
        newChannel.pathType = pathType;
        newChannel.samplerIndex = samplerIndex;

        // Find the node ID that maps to this joint
        int nodeId = -1;
        for (const auto& [node, joint] : skeleton->nodeJointMap) {
            if (joint == jointIndex) {
                nodeId = node;
                break;
            }
        }

        if (nodeId != -1) {
            newChannel.node = nodeId;
            // Add channel to animation
            animation.channels.push_back(newChannel);
        }
    }
    inline void SaveJointKeyframe(ve::Animation& animation, ve::Skeleton* skeleton, int jointIndex, float time,
                                  bool saveRotation, bool saveTranslation) {
        // Find channels for this joint
        bool rotationChannelFound = false;
        bool translationChannelFound = false;

        for (size_t i = 0; i < animation.channels.size(); i++) {
            auto& channel = animation.channels[i];

            // Check if this channel affects our joint
            if (skeleton->nodeJointMap[channel.node] == jointIndex) {
                auto& sampler = animation.samplers[channel.samplerIndex];

                // Handle rotation channel
                if (saveRotation && channel.pathType == ve::Animation::PathType::ROTATION) {
                    glm::vec4 rotationValue(
                            skeleton->joints[jointIndex].rotation.x,
                            skeleton->joints[jointIndex].rotation.y,
                            skeleton->joints[jointIndex].rotation.z,
                            skeleton->joints[jointIndex].rotation.w
                    );
                    UpdateOrCreateKeyframe(sampler, time, rotationValue);
                    rotationChannelFound = true;
                }

                    // Handle translation channel
                else if (saveTranslation && channel.pathType == ve::Animation::PathType::TRANSLATION) {
                    UpdateOrCreateKeyframe(sampler, time,
                                           glm::vec4(skeleton->joints[jointIndex].translation, 1.0f));
                    translationChannelFound = true;
                }
            }
        }

        // Create new channels if needed
        if (saveRotation && !rotationChannelFound) {
            glm::vec4 rotationValue(
                    skeleton->joints[jointIndex].rotation.x,
                    skeleton->joints[jointIndex].rotation.y,
                    skeleton->joints[jointIndex].rotation.z,
                    skeleton->joints[jointIndex].rotation.w
            );
            CreateAnimationChannel(animation, skeleton, jointIndex, time,
                                   ve::Animation::PathType::ROTATION, rotationValue);
        }

        if (saveTranslation && !translationChannelFound) {
            CreateAnimationChannel(animation, skeleton, jointIndex, time,
                                   ve::Animation::PathType::TRANSLATION,
                                   glm::vec4(skeleton->joints[jointIndex].translation, 1.0f));
        }

        // Update animation duration if needed
        if (time > animation.getLastKeyFrameTime()) {
            animation.setLastKeyFrameTime(time);
        } else if (time < animation.getFirstKeyFrameTime()) {
            animation.setFirstKeyFrameTime(time);
        }
    }
    inline void renderGameObjectDetails(VeGameObject& gameObject, bool& isHighlight, int& selectedJointIndex){
        ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
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
            float& currentTime = gameObject.model->animationManager->currentAnimation->currentKeyFrameTime;

            // Slider for animation scrubbing
            ImGui::Text("Animation: %s", currentAnim.c_str());

            // Animation progress slider
            bool timeChanged = ImGui::SliderFloat("Animation Time",
                                                  &gameObject.model->animationManager->currentAnimation->currentKeyFrameTime,
                                                  0.0f,
                                                  animationDuration,
                                                  "%.2f sec"
            );
            if(timeChanged){
                gameObject.model->animationManager->updateTimeSkip(*gameObject.model->skeleton);
            }
            std::vector<float> keyframeTimes = gameObject.model->animationManager->getKeyframeTimes();

            // Call the custom timeline widget
//            bool  timeLineEdit = TimelineWidget("##AnimationTimeline", &currentTime, animationDuration, keyframeTimes);
            bool timeLineEdit = ZoomableTimelineWidget("##AnimationTimeline", &currentTime, animationDuration, keyframeTimes);
            if(timeLineEdit){
                // Ensure time doesn't exceed duration slightly due to float inaccuracies if dragging
                currentTime = ImClamp(currentTime, 0.0f, animationDuration);
                gameObject.model->animationManager->updateTimeSkip(*gameObject.model->skeleton);
                // If animation was playing, maybe pause it on manual scrub? Optional.
                // isPaused = true;
                // gameObject.model->animationManager->stop();
            }
            static float playbackSpeed = 1.0f;
            ImGui::SliderFloat("Playback Speed", &gameObject.model->animationManager->currentAnimation->playbackSpeed, 0.1f, 3.0f, "%.1fx");
            // Playback controls
            ImGui::Columns(3, "AnimControls", false);

            // Rewind button
            if (ImGui::Button("Previous", ImVec2(-1, 0))) {
                currentTime = fmax(0.0f,currentTime-0.02f);
                gameObject.model->animationManager->updateTimeSkip(*gameObject.model->skeleton);
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
            if (ImGui::Button("Next", ImVec2(-1, 0))) {
                currentTime += 0.02f;
                gameObject.model->animationManager->updateTimeSkip(*gameObject.model->skeleton);
            }
            ImGui::Columns(1);
            bool isRepeat = gameObject.model->animationManager->isCurrentAnimRepeat();
            if (ImGui::Checkbox("Repeat Animation", &isRepeat)) {
                gameObject.model->animationManager->setRepeat();
            }

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
            if (ImGui::CollapsingHeader("Joint Editor", ImGuiTreeNodeFlags_DefaultOpen)) {
                static glm::vec3 originalPosition;
                static glm::vec3 editPosition;
                static bool editingJoint = false;

                // Only enable joint editing when animation is paused
                bool isPaused = !gameObject.model->animationManager->currentAnimation->isRunning();

                if (!isPaused) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                                       "Pause animation to edit joints");
                }

                // Get the skeleton pointer for convenience
                auto *skeleton = gameObject.model->skeleton.get();
                if (!skeleton) {
                    ImGui::Text("No skeleton available for this model");
                } else {
                    int numJoints = skeleton->joints.size();

                    // Joint selection dropdown
                    if (ImGui::BeginCombo("Select Joint",
                                          selectedJointIndex >= 0 ?
                                          skeleton->joints[selectedJointIndex].name.c_str() :
                                          "None")) {
                        for (int i = 0; i < numJoints; i++) {
                            bool isSelected = (selectedJointIndex == i);
                            if (ImGui::Selectable(skeleton->joints[i].name.c_str(), isSelected)) {
                                selectedJointIndex = i;

                                // When selecting a joint, capture its current position
                                if (isPaused && selectedJointIndex >= 0) {
                                    // Get the joint's global transform
                                    glm::mat4 globalTransform;
                                    if (skeleton->isAnimated) {
                                        globalTransform = skeleton->joints[selectedJointIndex].getLocalMatrix();

                                        // Apply parent transforms if not root
                                        int16_t parentIndex = skeleton->joints[selectedJointIndex].parentIndex;
                                        if (parentIndex != ve::NO_PARENT) {
                                            globalTransform = skeleton->jointMatrices[parentIndex] *
                                                              globalTransform;
                                        }
                                    } else {
                                        globalTransform = skeleton->joints[selectedJointIndex].jointWorldMatrix;
                                    }

                                    // Extract position
                                    originalPosition = glm::vec3(globalTransform[3]);
                                    editPosition = originalPosition;
                                    editingJoint = true;
                                }
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    //edit here
                    if (selectedJointIndex >= 0 && isPaused && editingJoint) {
                        ImGui::Separator();
                        ImGui::Text("Editing Joint: %s", skeleton->joints[selectedJointIndex].name.c_str());

                        bool positionChanged = false;
                        positionChanged |= ImGui::SliderFloat("Position X", &editPosition.x,
                                                              originalPosition.x - 20.0f,
                                                              originalPosition.x + 20.0f);
                        positionChanged |= ImGui::SliderFloat("Position Y", &editPosition.y,
                                                              originalPosition.y - 20.0f,
                                                              originalPosition.y + 20.0f);
                        positionChanged |= ImGui::SliderFloat("Position Z", &editPosition.z,
                                                              originalPosition.z - 20.0f,
                                                              originalPosition.z + 20.0f);

                        if (positionChanged) {
                            // Calculate where the joint should be in world space
                            glm::vec3 targetWorldPos = editPosition;

                            int16_t jointIndex = selectedJointIndex;

                            auto& joint = skeleton->joints[jointIndex];

                            if (joint.parentIndex != ve::NO_PARENT) {
                                // Convert target world position to local space relative to parent
                                glm::mat4 parentGlobalInv = glm::inverse(skeleton->jointMatrices[joint.parentIndex]);
                                glm::vec3 targetLocalPos = glm::vec3(parentGlobalInv * glm::vec4(targetWorldPos, 1.0f));

                                // Set the joint's local translation directly
                                joint.translation = targetLocalPos;
                            } else {
                                // Root joint - set position directly
                                joint.translation = targetWorldPos;
                            }

                            // Update the joint's transform matrices
                            skeleton->updateJoint(jointIndex);

                            // Update all child joints recursively
                            for (int i = 0; i < numJoints; i++) {
                                if (skeleton->joints[i].parentIndex == jointIndex) {
                                    skeleton->updateJoint(i);
                                }
                            }
                        }

                        // Button to save the modified pose
                        if (ImGui::Button("Save Modified Pose")) {
                            // Get the current animation and time
                            auto& animation = *gameObject.model->animationManager->currentAnimation;
                            float currentTime = animation.currentKeyFrameTime;

                            SaveJointKeyframe(animation, skeleton, selectedJointIndex, currentTime,
                                              false, true);

                            // Indicate success
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Pose saved!");
                        }

                        // Button to revert changes
                        if (ImGui::Button("Revert Changes")) {
                            auto& joint = skeleton->joints[selectedJointIndex];

                            // Restore original position
                            glm::vec3 offset = originalPosition - editPosition;

                            if (joint.parentIndex != ve::NO_PARENT) {
                                // Get parent's inverse global transform
                                glm::mat4 parentGlobalInv = glm::inverse(skeleton->jointMatrices[joint.parentIndex]);
                                // Convert world offset to local space
                                glm::vec3 localOffset = glm::vec3(parentGlobalInv * glm::vec4(offset, 0.0f));
                                joint.translation += localOffset;
                            } else {
                                // Root joint - apply offset directly
                                joint.translation += offset;
                            }

                            // Reset edit position
                            editPosition = originalPosition;

                            // Update the joint's transform matrices
                            skeleton->updateJoint(selectedJointIndex);

                            // Update all child joints recursively
                            for (int i = 0; i < numJoints; i++) {
                                if (skeleton->joints[i].parentIndex == selectedJointIndex) {
                                    skeleton->updateJoint(i);
                                }
                            }
                        }

                        // Add options for changing interpolation method
                        if (ImGui::CollapsingHeader("Keyframe Settings")) {
                            static int interpolationMethod = 0; // 0 = LINEAR, 1 = STEP
                            ImGui::Text("Interpolation Method:");
                            ImGui::RadioButton("Linear", &interpolationMethod, 0);
                            ImGui::SameLine();
                            ImGui::RadioButton("Step", &interpolationMethod, 1);

                            if (ImGui::Button("Apply to Current Keyframe")) {
                                // Get the current animation
                                auto& animation = *gameObject.model->animationManager->currentAnimation;
                                float currentTime = animation.currentKeyFrameTime;

                                // Find the channel for this joint
                                for (auto& channel : animation.channels) {
                                    if (skeleton->nodeJointMap[channel.node] == selectedJointIndex &&
                                        channel.pathType == ve::Animation::PathType::TRANSLATION) {

                                        // Update interpolation method
                                        animation.samplers[channel.samplerIndex].interpolationMethod =
                                                interpolationMethod == 0 ?
                                                ve::Animation::InterpolationMethod::LINEAR :
                                                ve::Animation::InterpolationMethod::STEP;

                                        break;
                                    }
                                }
                            }
                        }

                        // Add rotation and scale editing if needed
                        if (ImGui::CollapsingHeader("Advanced Properties")) {
                            // Add rotation editing UI here
                            // ...
                            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                                               "Feature to be implemented..");
                            // Add scale editing UI here
                            // ...
                        }
                        if (ImGui::CollapsingHeader("Inverse Kinematics")) {
                            static bool useIK = false;
                            static int ikChainLength = 2;
                            static float ikDamping = 0.5f;
                            static int maxIterations = 10;
                            static float targetThreshold = 0.01f;
                            static glm::vec3 ikTargetPosition(0.0f);
                            static std::vector<glm::quat> originalRotations;
                            static std::vector<int> ikChainJoints;
                            static bool ikChainInitialized = false;
                            static glm::vec3 ikOffset(0.0f);
                            static glm::vec3 initialEndEffectorPos(0.0f);
                            static bool isResetting = false;

                            // Local helper function for CCD solving
                            auto solveCCD = [](ve::Skeleton* skeleton, const std::vector<int>& ikChain,
                                               const glm::vec3& targetPos, int maxIterations,
                                               float damping, float threshold) {
                                for (int iteration = 0; iteration < maxIterations; iteration++) {
                                    // Work from the last joint in the chain to the first
                                    for (int i = ikChain.size() - 1; i >= 0; i--) {
                                        int jointIdx = ikChain[i];
                                        int endEffectorIdx = ikChain[0]; // First joint is our end effector

                                        // Need to recompute matrices at each step to get accurate positions
                                        skeleton->updateForIK();

                                        // Get joint and end effector positions in world space
                                        glm::vec3 jointPos = glm::vec3(skeleton->jointMatrices[jointIdx][3]);
                                        glm::vec3 endEffectorPos = glm::vec3(skeleton->jointMatrices[endEffectorIdx][3]);

                                        // Calculate vectors (in world space)
                                        glm::vec3 toEndEffector = endEffectorPos - jointPos;
                                        glm::vec3 toTarget = targetPos - jointPos;

                                        // Skip if vectors are too small
                                        if (glm::length(toEndEffector) < 0.001f || glm::length(toTarget) < 0.001f)
                                            continue;

                                        // Normalize vectors
                                        toEndEffector = glm::normalize(toEndEffector);
                                        toTarget = glm::normalize(toTarget);

                                        // Calculate rotation axis and angle
                                        float cosAngle = glm::dot(toEndEffector, toTarget);
                                        cosAngle = glm::clamp(cosAngle, -1.0f, 1.0f);
                                        float angle = std::acos(cosAngle) * damping;

                                        // Skip if angle is too small
                                        if (angle < 0.001f) continue;

                                        // Calculate rotation axis
                                        glm::vec3 rotAxis = glm::cross(toEndEffector, toTarget);
                                        if (glm::length(rotAxis) < 0.001f) continue;
                                        rotAxis = glm::normalize(rotAxis);

                                        // Create rotation quaternion in world space
                                        glm::quat worldRotation = glm::angleAxis(angle, rotAxis);

                                        // Convert to joint's local space
                                        auto& joint = skeleton->joints[jointIdx];

                                        // If joint has a parent, transform rotation from world to local space
                                        if (joint.parentIndex != ve::NO_PARENT) {
                                            // Get parent's world rotation
                                            glm::mat4 parentMat = skeleton->jointMatrices[joint.parentIndex];
                                            glm::quat parentWorldRotation = glm::quat_cast(glm::mat3(parentMat));

                                            // Convert world rotation to local space
                                            glm::quat localRot = glm::inverse(parentWorldRotation) * worldRotation * parentWorldRotation;

                                            // Apply to joint's current rotation
                                            joint.rotation = localRot * joint.rotation;
                                        } else {
                                            // Root joint - can apply world rotation directly
                                            joint.rotation = worldRotation * joint.rotation;
                                        }
                                    }

                                    // Check if we're close enough to target
                                    skeleton->updateForIK();
                                    glm::vec3 currentEndEffectorPos = glm::vec3(skeleton->jointMatrices[ikChain[0]][3]);
                                    float dist = glm::distance(currentEndEffectorPos, targetPos);
                                    if (dist < threshold) {
                                        break;
                                    }
                                }
                            };

                            // Toggle for IK mode
                            if (ImGui::Checkbox("Use IK when moving joint", &useIK)) {
                                if (useIK && selectedJointIndex >= 0) {
                                    // Store original joint rotations for ALL joints in the skeleton
                                    // This ensures we can properly reset the entire pose
                                    originalRotations.clear();
                                    for (size_t i = 0; i < skeleton->joints.size(); i++) {
                                        originalRotations.push_back(skeleton->joints[i].rotation);
                                    }

                                    // Build and store the IK chain
                                    ikChainJoints.clear();

                                    // Start with selected joint and add parents up the chain
                                    int currentJoint = selectedJointIndex;
                                    for (int i = 0; i < ikChainLength && currentJoint != ve::NO_PARENT; i++) {
                                        ikChainJoints.push_back(currentJoint);
                                        currentJoint = skeleton->joints[currentJoint].parentIndex;
                                    }

                                    // Update matrices to get accurate positions
                                    skeleton->updateForIK();

                                    // Initialize target position to current end effector position
                                    initialEndEffectorPos = glm::vec3(skeleton->jointMatrices[selectedJointIndex][3]);
                                    ikTargetPosition = initialEndEffectorPos;
                                    ikOffset = glm::vec3(0.0f);

                                    ikChainInitialized = true;
                                } else {
                                    // When disabling IK, restore original rotations
                                    if (ikChainInitialized && originalRotations.size() == skeleton->joints.size()) {
                                        for (size_t i = 0; i < skeleton->joints.size(); i++) {
                                            skeleton->joints[i].rotation = originalRotations[i];
                                        }
                                        skeleton->updateForIK();
                                    }
                                    ikChainInitialized = false;
                                }
                            }

                            if (useIK) {
                                ImGui::SliderInt("Chain Length", &ikChainLength, 1, 5);
                                ImGui::SliderFloat("Damping Factor", &ikDamping, 0.1f, 1.0f, "%.2f");
                                ImGui::SliderInt("Max Iterations", &maxIterations, 1, 20);
                                ImGui::SliderFloat("Target Threshold", &targetThreshold, 0.001f, 0.1f, "%.3f");

                                // Automatic reset when offset returns to zero
                                if (ImGui::SliderFloat("Offset X", &ikOffset.x, -20.0f, 20.0f) ||
                                    ImGui::SliderFloat("Offset Y", &ikOffset.y, -20.0f, 20.0f) ||
                                    ImGui::SliderFloat("Offset Z", &ikOffset.z, -20.0f, 20.0f)) {

                                    // Check if all offsets are near zero
                                    if (glm::length(ikOffset) < 0.01f && !isResetting) {
                                        // Reset to original pose when sliders return to zero
                                        isResetting = true;
                                        for (size_t i = 0; i < skeleton->joints.size(); i++) {
                                            skeleton->joints[i].rotation = originalRotations[i];
                                        }
                                        skeleton->updateForIK();
                                        ikTargetPosition = initialEndEffectorPos;
                                        isResetting = false;
                                    } else {
                                        // Calculate target position from offset relative to initial position
                                        ikTargetPosition = initialEndEffectorPos + ikOffset;

                                        // Update IK
                                        solveCCD(skeleton, ikChainJoints, ikTargetPosition, maxIterations,
                                                 ikDamping, targetThreshold);

                                        // Update the edit position
                                        skeleton->updateForIK();
                                        editPosition = glm::vec3(skeleton->jointMatrices[selectedJointIndex][3]);
                                    }
                                }

                                // Current positions display
                                glm::vec3 currentEndEffectorPos = glm::vec3(skeleton->jointMatrices[selectedJointIndex][3]);
                                ImGui::Text("Current End Effector: (%.3f, %.3f, %.3f)",
                                            currentEndEffectorPos.x, currentEndEffectorPos.y, currentEndEffectorPos.z);
                                ImGui::Text("Target Position: (%.3f, %.3f, %.3f)",
                                            ikTargetPosition.x, ikTargetPosition.y, ikTargetPosition.z);
                                ImGui::Text("Distance to Target: %.3f",
                                            glm::distance(currentEndEffectorPos, ikTargetPosition));
                                // In the IK section, add a "Save IK Pose" button:
                                if (useIK && ImGui::Button("Save IK Pose as Keyframe")) {
                                    // Get the current animation and time
                                    auto& animation = *gameObject.model->animationManager->currentAnimation;
                                    float currentTime = animation.currentKeyFrameTime;

                                    // Save keyframes for all joints in the IK chain
                                    for (int jointIdx : ikChainJoints) {
                                        // IK primarily affects rotation, so save both rotation and translation
                                        SaveJointKeyframe(animation, skeleton, jointIdx, currentTime,
                                                          true, true); // Save both rotation and translation
                                    }

                                    // Indicate success
                                    ImGui::SameLine();
                                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "IK pose saved!");
                                }
                                // Reset button
                                if (ImGui::Button("Reset to Original Pose")) {
                                    // Restore original rotations for all joints
                                    if (ikChainInitialized && originalRotations.size() == skeleton->joints.size()) {
                                        for (size_t i = 0; i < skeleton->joints.size(); i++) {
                                            skeleton->joints[i].rotation = originalRotations[i];
                                        }
                                        skeleton->updateForIK();
                                        ikOffset = glm::vec3(0.0f);
                                        ikTargetPosition = initialEndEffectorPos;
                                    }
                                }
                            }
                        }
                    } else if (selectedJointIndex >= 0 && !isPaused) {
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                                           "Pause animation to edit this joint");
                    }
                }
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