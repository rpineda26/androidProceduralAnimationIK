//
// Created by Ralph Dawson Pineda on 8/6/25.
//
#include "animation_sequencer.hpp"
#include "utility.hpp"
#include "debug.hpp"

namespace ve {

    AnimationSequencer::AnimationSequencer()
            : jointEditor(std::make_unique<JointEditor>()) {
        LOGI("AnimationSequencer::AnimationSequencer() Constructed");
    }

    void AnimationSequencer::ShowSequencerWindow(VeGameObject& gameObject, EngineInfo& engineInfo) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.y * 0.5f, 300.0f),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1500, 600),ImGuiCond_FirstUseEver);
        ImGuiWindowFlags window_flags = 0;
        if (!ImGui::Begin("Animation Sequencer")) {
            ImGui::End();
            return;
        }

        // Safety check - but don't return early, show the sequencer anyway
        bool hasValidModel = gameObject.model && gameObject.model->skeleton &&
                             !gameObject.model->skeleton->joints.empty();

        if (!hasValidModel) {
            ImGui::Text("No model or skeleton loaded");
            ImGui::End();
            return;
        }

        // Check if we need to reinitialize (e.g., animation changed)
        bool needsReinit = !isInitialized;
        if (gameObject.model->animationManager && gameObject.model->animationManager->currentAnimation) {
            static Animation* lastAnimation = nullptr;
            if (lastAnimation != gameObject.model->animationManager->currentAnimation) {
                needsReinit = true;
                lastAnimation = gameObject.model->animationManager->currentAnimation;
            }
        }

        if (needsReinit) {
            SetupJointNames(gameObject);
        }

        // Animation selection dropdown
        if (gameObject.model->animationManager && gameObject.model->animationManager->size() > 0) {
            ImGui::Text("Current Animation: %s", gameObject.model->animationManager->getName().c_str());

            if (ImGui::BeginCombo("Select Animation", gameObject.model->animationManager->getName().c_str())) {
                auto allNames = gameObject.model->animationManager->getAllNames();
                for (size_t i = 0; i < allNames.size(); i++) {
                    bool isSelected = (gameObject.model->animationManager->getName() == allNames[i]);
                    if (ImGui::Selectable(allNames[i].c_str(), isSelected)) {
                        gameObject.model->animationManager->start(allNames[i]);
                        // Reinitialize sequencer with new animation data
                        SetupJointNames(gameObject);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();
        }

        ShowSequencerControls(gameObject, engineInfo, hasValidModel);

        // === SEQUENCER REGION ===
        ImGui::Separator();
        ImGui::Text("Animation Timeline");

        const float SEQUENCER_HEIGHT = 1400.0f;


        ImGui::BeginChild("SequencerRegion", ImVec2(0, SEQUENCER_HEIGHT), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        if (ImSequencer::Sequencer(this,
                                   &currentFrame,
                                   &expanded,
                                   &selectedEntry,
                                   &firstFrame,
                                   ImSequencer::SEQUENCER_EDIT_STARTEND |
                                   ImSequencer::SEQUENCER_ADD |
                                   ImSequencer::SEQUENCER_DEL |
                                   ImSequencer::SEQUENCER_CHANGE_FRAME)) {
            ApplyPoseAtFrame(gameObject, engineInfo, currentFrame);
        }

        ImGui::EndChild();
        // Rest of the UI...
        ImGui::Separator();


        if (selectedEntry >= 0) {
            int jointIndex = GetVisibleJointIndex(selectedEntry);
            if (jointIndex >= 0 && jointIndex < jointNames.size()) {
                ImGui::Separator();
                ImGui::Text("Selected joint: %s", jointNames[jointIndex].c_str());
                engineInfo.selectedJointIndex = jointIndex;

                ImGui::Text("Use the 3D gizmo to manipulate this joint");
                if (ImGui::Button("Create Keyframe for This Joint")) {
                    AddKeyframeForJoint(gameObject, jointIndex);
                }
            }
        }

        ShowKeyframeDetails(gameObject, engineInfo);

        ImGui::End();
    }

// Fixed DoubleClick - remove gameObject parameter dependency
    void AnimationSequencer::DoubleClick(int index) {
        selectedEntry = index;
        int jointIndex = GetVisibleJointIndex(index);
        if (jointIndex >= 0) {
            // Find nearest keyframe and jump to it
            const auto &track = jointTracks[jointIndex];
            int nearestFrame = currentFrame;
            int minDistance = INT_MAX;

            for (const auto &kf: track.keyframes) {
                int distance = abs(kf.frame - currentFrame);
                if (distance < minDistance) {
                    minDistance = distance;
                    nearestFrame = kf.frame;
                }
            }
            currentFrame = nearestFrame;
        }
    }

    void AnimationSequencer::ShowJointEditorWindow(VeGameObject& gameObject, EngineInfo& engineInfo) {
        jointEditor->ShowJointEditorWindow(gameObject, engineInfo);
    }

    void AnimationSequencer::RenderJointGizmo(VeGameObject& gameObject, EngineInfo& engineInfo) {
        jointEditor->RenderGizmo(gameObject, engineInfo);
    }

    void AnimationSequencer::BeginFrame() {
        jointEditor->BeginFrame();
    }

    int AnimationSequencer::GetCurrentFrame() const {
        return currentFrame;
    }

    void AnimationSequencer::SetCurrentFrame(int frame) {
        currentFrame = frame;
    }

    bool AnimationSequencer::HasSelectedKeyframe() const {
        return selectedKeyframe >= 0;
    }

    int AnimationSequencer::GetSelectedKeyframe() const {
        return selectedKeyframe;
    }

    void AnimationSequencer::SetupJointNames(VeGameObject& gameObject) {
        if (!gameObject.model || !gameObject.model->skeleton) {
            return;
        }

        auto* skeleton = gameObject.model->skeleton.get();
        auto* animationManager = gameObject.model->animationManager.get();

        jointNames.clear();
        jointTracks.clear();
        usedAbbreviations.clear();

        // Create one track per joint with abbreviated names
        for (size_t i = 0; i < skeleton->joints.size(); i++) {
            const auto& joint = skeleton->joints[i];
            std::string abbreviatedName = CreateAbbreviatedName(joint.name, i);
            jointNames.push_back(abbreviatedName);

            JointTrack track;

            // Always add a default keyframe at frame 0 with current joint transform
            track.AddKeyframe(0, joint.translation, joint.rotation, joint.scale);

            // If we have animation data, extract keyframes for this joint
            if (animationManager && animationManager->currentAnimation) {
                ExtractKeyframesForJoint(track, i, *animationManager->currentAnimation, gameObject);
            }

            jointTracks.push_back(std::move(track));
        }

        isInitialized = true;
        needsReflatten = true; // Trigger re-flattening since we have new data
        LOGI("Setup %zu joint tracks with animation keyframes\n", jointTracks.size());
    }
    void AnimationSequencer::ExtractKeyframesForJoint(JointTrack& track, int jointIndex, const Animation& animation, VeGameObject& gameObject) {
        std::vector<float> keyframeTimes = gameObject.model->animationManager->currentAnimation->getKeyframeTimes();

        // Temporary structure to hold interpolated TRS values per frame
        struct TRS {
            glm::vec3 translation = glm::vec3(0.0f);
            glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 scale = glm::vec3(1.0f);
            bool hasTranslation = false;
            bool hasRotation = false;
            bool hasScale = false;
        };

        std::map<int, TRS> frameToTRSMap;  // frame index -> TRS
        int frameMax = 0;

        for (const auto& channel : animation.channels) {
            if (channel.node != jointIndex) continue;

            const auto& sampler = animation.samplers[channel.samplerIndex];
            const auto& times = sampler.timeStamps;

            for (size_t i = 0; i + 1 < times.size(); ++i) {
                float t0 = times[i];
                float t1 = times[i + 1];

                // Sample 3 intermediate points: start, mid, end
                std::vector<float> sampleTimes = { t0, (t0 + t1) * 0.5f, t1 };

                for (float sampleTime : sampleTimes) {
                    float t = (sampleTime - t0) / (t1 - t0);  // normalized time
                    int frame = static_cast<int>(sampleTime * 30.0f);
                    frameMax = std::max(frameMax, frame);

                    glm::vec4 v0 = sampler.TRSoutputValues[i];
                    glm::vec4 v1 = sampler.TRSoutputValues[i + 1];

                    TRS& trs = frameToTRSMap[frame];

                    switch (channel.pathType) {
                        case Animation::PathType::TRANSLATION:
                            trs.translation = glm::mix(glm::vec3(v0), glm::vec3(v1), t);
                            trs.hasTranslation = true;
                            break;

                        case Animation::PathType::ROTATION: {
                            glm::quat q1(v0.w, v0.x, v0.y, v0.z);
                            glm::quat q2(v1.w, v1.x, v1.y, v1.z);
                            trs.rotation = glm::normalize(glm::slerp(q1, q2, t));
                            trs.hasRotation = true;
                            break;
                        }

                        case Animation::PathType::SCALE:
                            trs.scale = glm::mix(glm::vec3(v0), glm::vec3(v1), t);
                            trs.hasScale = true;
                            break;
                    }
                }
            }
        }

        // Add accumulated keyframes to the joint track
        for (const auto& [frame, trs] : frameToTRSMap) {
            track.AddKeyframe(frame, trs.translation, trs.rotation, trs.scale);
        }

        // Optionally store max frame range somewhere
        // this->frameMax = std::max(this->frameMax, frameMax); // If needed
    }

    std::string AnimationSequencer::CreateAbbreviatedName(const std::string& fullName, size_t index) {
        // If name is already short enough, keep it
        static const std::unordered_map<std::string, std::string> jointMap = {
                {"root", "root"}, {"Root", "root"},
                {"spine", "spin"}, {"Spine", "spin"},
                {"head", "head"}, {"Head", "head"},
                {"neck", "neck"}, {"Neck", "neck"},
                {"hips", "hips"}, {"Hips", "hips"},
                {"pelvis", "pelv"}, {"Pelvis", "pelv"},
                {"lefthand", "lhnd"}, {"LeftHand", "lhnd"}, {"left_hand", "lhnd"},
                {"righthand", "rhnd"}, {"RightHand", "rhnd"}, {"right_hand", "rhnd"},
                {"leftfoot", "lft"}, {"LeftFoot", "lft"}, {"left_foot", "lft"},
                {"rightfoot", "rft"}, {"RightFoot", "rft"}, {"right_foot", "rft"},
                {"leftarm", "larm"}, {"LeftArm", "larm"}, {"left_arm", "larm"},
                {"rightarm", "rarm"}, {"RightArm", "rarm"}, {"right_arm", "rarm"},
                {"leftleg", "lleg"}, {"LeftLeg", "lleg"}, {"left_leg", "lleg"},
                {"rightleg", "rleg"}, {"RightLeg", "rleg"}, {"right_leg", "rleg"},
                // Dog body parts
                {"tongue", "tong"}, {"Tongue", "tong"},
                {"nose", "nose"}, {"Nose", "nose"},
                {"snout", "snot"}, {"Snout", "snot"},
                {"muzzle", "muzz"}, {"Muzzle", "muzz"},
                {"mouth", "mout"}, {"Mouth", "mout"},
                {"eye", "eye"}, {"Eye", "eye"},
                {"lefteye", "leye"}, {"LeftEye", "leye"}, {"left_eye", "leye"},
                {"righteye", "reye"}, {"RightEye", "reye"}, {"right_eye", "reye"},
                {"jaw", "jaw"}, {"Jaw", "jaw"},
                {"ear", "ear"}, {"Ear", "ear"},
                {"leftear", "lear"}, {"LeftEar", "lear"}, {"left_ear", "lear"},
                {"rightear", "rear"}, {"RightEar", "rear"}, {"right_ear", "rear"},
                {"tail", "tail"}, {"Tail", "tail"},
                // Paws and claws
                {"paw", "paw"}, {"Paw", "paw"},
                {"leftpaw", "lpaw"}, {"LeftPaw", "lpaw"}, {"left_paw", "lpaw"},
                {"rightpaw", "rpaw"}, {"RightPaw", "rpaw"}, {"right_paw", "rpaw"},
                {"claw", "claw"}, {"Claw", "claw"},
                {"toe", "toe"}, {"Toe", "toe"},
                // Legs (more specific for quadrupeds)
                {"frontleg", "fleg"}, {"FrontLeg", "fleg"},
                {"backleg", "bleg"}, {"BackLeg", "bleg"},
                {"shoulder", "shld"}, {"Shoulder", "shld"},
                {"elbow", "elbw"}, {"Elbow", "elbw"},
                {"knee", "knee"}, {"Knee", "knee"},
                {"ankle", "ankl"}, {"Ankle", "ankl"},
                {"wrist", "wrst"}, {"Wrist", "wrst"}
        };

        std::string name = fullName;

        // Remove common prefixes
        const std::string prefixes[] = {"mixamorig:", "mixamorig_", "Armature_", "joint_", "bone_"};
        for (const auto& prefix : prefixes) {
            if (name.substr(0, prefix.length()) == prefix) {
                name = name.substr(prefix.length());
                break;
            }
        }

        std::string abbreviated;

        // Check for exact matches first
        auto it = jointMap.find(name);
        if (it != jointMap.end()) {
            abbreviated = it->second;
        }
            // Handle numbered spine joints (spine-01, spine_02, etc.)
        else if (name.find("spine") == 0 || name.find("Spine") == 0) {
            std::string numStr;
            for (char c : name) {
                if (std::isdigit(c)) numStr += c;
            }
            abbreviated = numStr.empty() ? "spin" : "sp" + numStr.substr(0, 2);
        }
            // Default: take first 4 chars, lowercase
        else {
            abbreviated = name.substr(0, 4);
            std::transform(abbreviated.begin(), abbreviated.end(), abbreviated.begin(), ::tolower);
        }

        // Ensure exactly 4 characters
        abbreviated.resize(4, '0');

        // Make unique by appending counter if needed
        std::string result = abbreviated;
        int counter = 1;
        while (usedAbbreviations.count(result)) {
            if (counter < 10) {
                result = abbreviated.substr(0, 3) + std::to_string(counter);
            } else {
                result = abbreviated.substr(0, 2) + std::to_string(counter).substr(0, 2);
            }
            counter++;
        }

        usedAbbreviations.insert(result);
        return result;
    }

    void AnimationSequencer::ShowSequencerControls(VeGameObject& gameObject, EngineInfo& engineInfo, bool hasValidModel) {
        if (hasValidModel && gameObject.model->animationManager &&
            gameObject.model->animationManager->currentAnimation) {

            if (ImGui::Button("Play")) {
                gameObject.model->animationManager->currentAnimation->start();
            }
            ImGui::SameLine();
            if (ImGui::Button("Pause")) {
                gameObject.model->animationManager->currentAnimation->stop();
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                currentFrame = 0;
                ApplyPoseAtFrame(gameObject,engineInfo,currentFrame);
            }
        }

        ImGui::Separator();

        if (hasValidModel) {
            if (ImGui::Button("Add Keyframe for Selected Joint")) {
                if (engineInfo.selectedJointIndex >= 0) {
                    AddKeyframeForJoint(gameObject, engineInfo.selectedJointIndex);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Add Keyframes for All Joints")) {
                for (size_t i = 0; i < jointTracks.size(); i++) {
                    AddKeyframeForJoint(gameObject, (int)i);
                }
            }

            ImGui::SameLine();
        }
    }

    void AnimationSequencer::ShowJointVisibilityControls() {
        if (ImGui::CollapsingHeader("Joint Visibility")) {
            for (size_t i = 0; i < jointNames.size() && i < jointTracks.size(); i++) {
                ImGui::Checkbox(jointNames[i].c_str(), &jointTracks[i].visible);
                if ((i + 1) % 4 != 0) ImGui::SameLine();
            }
        }
    }

    void AnimationSequencer::ShowKeyframeDetails(VeGameObject& gameObject, EngineInfo& engineInfo) {
        if (selectedEntry < 0 || selectedEntry >= jointTracks.size()) return;

        ImGui::Separator();
        ImGui::Text("Joint: %s", jointNames[selectedEntry].c_str());

        auto& track = jointTracks[selectedEntry];
        ImGui::Text("Keyframes: %zu", track.keyframes.size());

        // Show keyframes for this joint
        if (ImGui::BeginTable("Keyframes", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Frame");
            ImGui::TableSetupColumn("Translation");
            ImGui::TableSetupColumn("Rotation");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < track.keyframes.size(); i++) {
                auto& kf = track.keyframes[i];
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%d", kf.frame);

                ImGui::TableNextColumn();
                ImGui::Text("%.2f, %.2f, %.2f", kf.translation.x, kf.translation.y, kf.translation.z);

                ImGui::TableNextColumn();
                ImGui::Text("%.2f, %.2f, %.2f, %.2f", kf.rotation.w, kf.rotation.x, kf.rotation.y, kf.rotation.z);

                ImGui::TableNextColumn();
                ImGui::PushID((int)i);
                if (ImGui::Button("Edit")) {
                    currentFrame = kf.frame;
                    ApplyPoseAtFrame(gameObject, engineInfo ,currentFrame);
                }
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        auto& state = jointEditor->state;
        auto skeleton = gameObject.model->skeleton.get();
        if (state.selectedJoint >= 0 && state.editingJoint) {
            ImGui::Separator();
            ImGui::Text("Editing Joint: %s", skeleton->joints[state.selectedJoint].name.c_str());

            // Gizmo operation selection
            if (ImGui::RadioButton("Translate", state.currentOperation == ImGuizmo::TRANSLATE))
                state.currentOperation = ImGuizmo::TRANSLATE;
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", state.currentOperation == ImGuizmo::ROTATE))
                state.currentOperation = ImGuizmo::ROTATE;

            // Mode selection
            if (ImGui::RadioButton("World", state.currentMode == ImGuizmo::WORLD))
                state.currentMode = ImGuizmo::WORLD;
            ImGui::SameLine();
            if (ImGui::RadioButton("Local", state.currentMode == ImGuizmo::LOCAL))
                state.currentMode = ImGuizmo::LOCAL;

            ImGui::Separator();

            // Control buttons
            if (ImGui::Button("Save Modified Pose")) {
                jointEditor->SaveJointPose(gameObject, state.selectedJoint);
            }

            // Show save success message
            if (state.showSaveSuccess) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Pose saved!");
            }

            if (ImGui::Button("Revert Changes")) {
                jointEditor->RevertJointChanges(gameObject, state.selectedJoint, engineInfo);
            }
        }
    }

    void AnimationSequencer::AddKeyframeForJoint(VeGameObject& gameObject, int jointIndex) {
        if (jointIndex < 0 || jointIndex >= jointTracks.size()) return;

        auto* skeleton = gameObject.model->skeleton.get();
        if (!skeleton || jointIndex >= skeleton->joints.size()) return;

        const auto& joint = skeleton->joints[jointIndex];
        jointTracks[jointIndex].AddKeyframe(currentFrame,
                                            joint.translation, joint.rotation, joint.scale);

        needsReflatten = true; // Trigger re-flattening
        LOGI("Added keyframe for joint %d at frame %d\n", jointIndex, currentFrame);
    }
    // Remove keyframe for specific joint at current frame
    void AnimationSequencer::RemoveKeyframeForJoint(int jointIndex, int frame) {
        if (jointIndex < 0 || jointIndex >= jointTracks.size()) return;

        int targetFrame = (frame == -1) ? currentFrame : frame;
        jointTracks[jointIndex].RemoveKeyframe(targetFrame);
        needsReflatten = true; // Trigger re-flattening
        LOGI("Removed keyframe for joint %d at frame %d\n", jointIndex, targetFrame);
    }

    // Apply pose from all joint tracks at current frame
    void AnimationSequencer::ApplyPoseAtFrame(VeGameObject& gameObject, EngineInfo &engineInfo, int frame) {
        auto* skeleton = gameObject.model->skeleton.get();
        if (!skeleton) return;

        // Convert frame to time (assuming you have a way to get frame time)
        float timeAtFrame = float(frame); // You'll need to implement this
        auto currentAnim = gameObject.model->animationManager->currentAnimation;
        // Use the same logic as setAnimationTime to ensure consistency
        float duration = gameObject.model->animationManager->getDuration(currentAnim->getName()); // Use your existing method
        timeAtFrame = std::max(0.0f, std::min(timeAtFrame, duration));

        // Set the animation time and update through the animation manager
        gameObject.model->animationManager->currentAnimation->currentKeyFrameTime = timeAtFrame;
        gameObject.model->animationManager->stop();

        // Force update animation to the new time immediately
        gameObject.model->animationManager->updateTimeSkip(*gameObject.model->skeleton);
        jointEditor->state.editingJoint = true;
        jointEditor->state.selectedJoint = selectedEntry;
        engineInfo.selectedJointIndex = selectedEntry;
        engineInfo.showEditJointMode = true;
    }

// ImSequencer::SequenceInterface implementation
    int AnimationSequencer::GetFrameMin() const { return frameMin; }
    int AnimationSequencer::GetFrameMax() const { return frameMax; }
    int AnimationSequencer::GetItemTypeCount() const { return 1; }
    const char* AnimationSequencer::GetItemTypeName(int typeIndex) const {
        return "Joint Pose";
    }

    int AnimationSequencer::GetItemCount() const {
        int visibleJointCount = 0;
        for (const auto& track : jointTracks) {
            if (track.visible) {
                visibleJointCount++;
            }
        }
        return visibleJointCount;
    }

    const char* AnimationSequencer::GetItemLabel(int index) const {
        int jointIndex = GetVisibleJointIndex(index);
        if (jointIndex >= 0 && jointIndex < jointNames.size()) {
            return jointNames[jointIndex].c_str();
        }
        return "Invalid";
    }

    void AnimationSequencer::Get(int index, int** start, int** end, int* type, unsigned int* color) {
        int jointIndex = GetVisibleJointIndex(index);
        if (jointIndex < 0 || jointIndex >= jointTracks.size()) return;

        const auto& track = jointTracks[jointIndex];

        // For now, we'll show the track's full range
        // You might want to modify this to show individual keyframes as separate blocks
        static std::vector<int> startFrames, endFrames;
        if (startFrames.size() <= index) {
            startFrames.resize(index + 1);
            endFrames.resize(index + 1);
        }

        if (!track.keyframes.empty()) {
            int minFrame, maxFrame;
            track.GetFrameRange(minFrame, maxFrame);
            startFrames[index] = minFrame;
            endFrames[index] = maxFrame;
        } else {
            startFrames[index] = 0;
            endFrames[index] = 0;
        }

        if (start) *start = &startFrames[index];
        if (end) *end = &endFrames[index];
        if (type) *type = 0;
        if (color) {
            static const unsigned int colors[] = {
                    0xFFFF4444, 0xFF44FF44, 0xFF4444FF, 0xFFFFFF44,
                    0xFFFF44FF, 0xFF44FFFF, 0xFFAAAAAA, 0xFFFFAAAA
            };
            *color = colors[jointIndex % 8];
        }
    }


    void AnimationSequencer::Add(int type) {
        // This gets called when user clicks the "+" button in sequencer
        if (selectedEntry >= 0 && selectedEntry < jointTracks.size()) {
            // Add to selected joint only
            // Note: we need access to gameObject here, which is a limitation
            // You might want to store a reference or pass it differently
        }
    }

    void AnimationSequencer::Del(int index) {
        if (needsReflatten) {
            ReflattenKeyframes();
        }

        if (index < 0 || index >= flattenedKeyframes.size()) return;

        const auto& flattened = flattenedKeyframes[index];
        RemoveKeyframeForJoint(flattened.jointIndex, flattened.frame);
        needsReflatten = true; // Mark for re-flattening
    }

    void AnimationSequencer::Duplicate(int index) {
        int jointIndex = GetVisibleJointIndex(index);
        if (jointIndex < 0 || jointIndex >= jointTracks.size()) return;

        auto& track = jointTracks[jointIndex];
        auto* keyframe = track.GetKeyframeAtFrame(currentFrame);
        if (keyframe) {
            // Duplicate to a nearby frame
            track.AddKeyframe(currentFrame + 10,
                              keyframe->translation, keyframe->rotation, keyframe->scale);
        }
    }
    int AnimationSequencer::GetVisibleJointIndex(int timelineIndex) const {
        int visibleCount = 0;
        for (size_t i = 0; i < jointTracks.size(); i++) {
            if (jointTracks[i].visible) {
                if (visibleCount == timelineIndex) {
                    return static_cast<int>(i);
                }
                visibleCount++;
            }
        }
        return -1;
    }

    void AnimationSequencer::ReflattenKeyframes() {
        flattenedKeyframes.clear();

        for (int jointIdx = 0; jointIdx < jointTracks.size(); jointIdx++) {
            if (!jointTracks[jointIdx].visible) continue;

            for (const auto& keyframe : jointTracks[jointIdx].keyframes) {
                FlattenedKeyframe flattened;
                flattened.jointIndex = jointIdx;
                flattened.frame = keyframe.frame;
                flattened.translation = keyframe.translation;
                flattened.rotation = keyframe.rotation;
                flattened.scale = keyframe.scale;
                flattenedKeyframes.push_back(flattened);
            }
        }

        // Sort by joint index, then by frame for better organization
        std::sort(flattenedKeyframes.begin(), flattenedKeyframes.end(),
                  [](const FlattenedKeyframe& a, const FlattenedKeyframe& b) {
                      if (a.jointIndex != b.jointIndex) {
                          return a.jointIndex < b.jointIndex;
                      }
                      return a.frame < b.frame;
                  });

        needsReflatten = false;
    }
} // namespace ve