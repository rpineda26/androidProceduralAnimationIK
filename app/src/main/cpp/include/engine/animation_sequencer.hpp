//
// Created by Ralph Dawson Pineda on 8/6/25.
//

#ifndef VULKANANDROID_ANIMATION_SEQUENCER_HPP
#define VULKANANDROID_ANIMATION_SEQUENCER_HPP
#include "joint_editor.hpp"
#include "imgui.h"
#include "ImSequencer.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_set>


namespace ve {

    // Individual keyframe for a specific joint at a specific time
    struct JointKeyframe {
        int frame;
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};

        // Optional: easing/interpolation type
        enum class InterpolationType {
            LINEAR,
            BEZIER,
            STEP
        } interpolation = InterpolationType::LINEAR;
    };

// Track containing all keyframes for a single joint
    struct JointTrack {
        std::vector<JointKeyframe> keyframes;
        bool visible = true;

        // Add a keyframe at the specified frame
        void AddKeyframe(int frame, const glm::vec3& translation,
                         const glm::quat& rotation, const glm::vec3& scale) {
            // Check if keyframe already exists at this frame
            auto it = std::find_if(keyframes.begin(), keyframes.end(),
                                   [frame](const JointKeyframe& kf) { return kf.frame == frame; });

            if (it != keyframes.end()) {
                // Update existing keyframe
                it->translation = translation;
                it->rotation = rotation;
                it->scale = scale;
            } else {
                // Add new keyframe and keep sorted by frame
                JointKeyframe newKf;
                newKf.frame = frame;
                newKf.translation = translation;
                newKf.rotation = rotation;
                newKf.scale = scale;

                keyframes.push_back(newKf);
                std::sort(keyframes.begin(), keyframes.end(),
                          [](const JointKeyframe& a, const JointKeyframe& b) {
                              return a.frame < b.frame;
                          });
            }
        }

        // Remove keyframe at specific frame
        void RemoveKeyframe(int frame) {
            keyframes.erase(
                    std::remove_if(keyframes.begin(), keyframes.end(),
                                   [frame](const JointKeyframe& kf) { return kf.frame == frame; }),
                    keyframes.end());
        }

        // Get keyframe at specific frame (returns nullptr if not found)
        JointKeyframe* GetKeyframeAtFrame(int frame) {
            auto it = std::find_if(keyframes.begin(), keyframes.end(),
                                   [frame](const JointKeyframe& kf) { return kf.frame == frame; });
            return (it != keyframes.end()) ? &(*it) : nullptr;
        }

        // Get interpolated pose at any frame
        void GetInterpolatedPose(int frame, glm::vec3& outTranslation,
                                 glm::quat& outRotation, glm::vec3& outScale) const {
            if (keyframes.empty()) {
                outTranslation = glm::vec3(0.0f);
                outRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                outScale = glm::vec3(1.0f);
                return;
            }

            // Find surrounding keyframes
            const JointKeyframe* prevKf = nullptr;
            const JointKeyframe* nextKf = nullptr;

            for (const auto& kf : keyframes) {
                if (kf.frame <= frame) {
                    prevKf = &kf;
                }
                if (kf.frame >= frame && nextKf == nullptr) {
                    nextKf = &kf;
                    break;
                }
            }

            // Handle edge cases
            if (prevKf == nullptr) {
                // Before first keyframe
                const auto& kf = keyframes[0];
                outTranslation = kf.translation;
                outRotation = kf.rotation;
                outScale = kf.scale;
                return;
            }

            if (nextKf == nullptr || prevKf->frame == frame) {
                // After last keyframe or exactly on a keyframe
                outTranslation = prevKf->translation;
                outRotation = prevKf->rotation;
                outScale = prevKf->scale;
                return;
            }

            // Interpolate between keyframes
            float t = (float)(frame - prevKf->frame) / (float)(nextKf->frame - prevKf->frame);
            t = glm::clamp(t, 0.0f, 1.0f);

            outTranslation = glm::mix(prevKf->translation, nextKf->translation, t);
            outRotation = glm::slerp(prevKf->rotation, nextKf->rotation, t);
            outScale = glm::mix(prevKf->scale, nextKf->scale, t);
        }

        // Get frame range of all keyframes
        void GetFrameRange(int& minFrame, int& maxFrame) const {
            if (keyframes.empty()) {
                minFrame = maxFrame = 0;
                return;
            }

            minFrame = keyframes.front().frame;
            maxFrame = keyframes.back().frame;
        }

    };

    struct FlattenedKeyframe {
        int jointIndex;
        int frame;
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
    };
    struct TRS {
        glm::vec3 translation = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity quaternion
        glm::vec3 scale = glm::vec3(1.0f);
        bool hasTranslation = false;
        bool hasRotation = false;
        bool hasScale = false;
    };

    class AnimationSequencer : public ImSequencer::SequenceInterface {
    private:
        std::vector<std::string> jointNames;
        std::vector<JointTrack> jointTracks;
        std::vector<FlattenedKeyframe> flattenedKeyframes;
        bool needsReflatten = true;

        int selectedKeyframe = -1;     // Which keyframe is selected in our logic
        int selectedEntry = -1;        // Which timeline track is selected in ImSequencer
        int currentFrame = 0;
        int firstFrame = 0;            // First visible frame in timeline
        bool expanded = true;

        int frameMin = 0;
        int frameMax = 10; // 10 seconds at 30fps
        bool isInitialized = false;

        std::unique_ptr<JointEditor> jointEditor;
        std::unordered_set<std::string> usedAbbreviations;
        std::unordered_map<int, TRS> nodeTRSMap;

    public:
        AnimationSequencer();

        ~AnimationSequencer() = default;

        // Main method called from your main loop
        void ShowSequencerWindow(VeGameObject &gameObject, EngineInfo &engineInfo);

        // Joint editor integration
        void ShowJointEditorWindow(VeGameObject &gameObject, EngineInfo &engineInfo);

        void RenderJointGizmo(VeGameObject &gameObject, EngineInfo &engineInfo);

        void BeginFrame(); // Forward to joint editor

        // Frame management
        int GetCurrentFrame() const;

        void SetCurrentFrame(int frame);

        // Keyframe management
        bool HasSelectedKeyframe() const;

        int GetSelectedKeyframe() const;

        void AddKeyframeForJoint(VeGameObject &gameObject, int jointIndex);

        void RemoveKeyframeForJoint(int jointIndex, int frame);

    private:
        // Setup and utility methods
        void SetupJointNames(VeGameObject &gameObject);
        void ExtractKeyframesForJoint(JointTrack& track, int jointIndex, const Animation& animation, VeGameObject& gameObject);

        void ShowKeyframeDetails(VeGameObject &gameObject, EngineInfo &engineInfo);

        void
        ShowSequencerControls(VeGameObject &gameObject, EngineInfo &engineInfo, bool hasValidModel);

        void ShowJointVisibilityControls();

        void ApplyPoseAtFrame(VeGameObject &gameObject, EngineInfo &engineInfo, int frame);


        // ImSequencer::SequenceInterface implementation
        int GetFrameMin() const override;

        int GetFrameMax() const override;

        int GetItemCount() const override;

        int GetItemTypeCount() const override;

        const char *GetItemTypeName(int typeIndex) const override;

        const char *GetItemLabel(int index) const override;

        void Get(int index, int **start, int **end, int *type, unsigned int *color) override;

        void Add(int type) override;

        void Del(int index) override;

        void Duplicate(int index) override;

        void DoubleClick(int index) override;


        int GetVisibleJointIndex(int timelineIndex) const;
        std::string CreateAbbreviatedName(const std::string& fullName, size_t index);
        void ReflattenKeyframes();
    };

} // namespace ve
#endif //VULKANANDROID_ANIMATION_SEQUENCER_HPP

