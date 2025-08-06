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

}