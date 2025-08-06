//
// Created by Ralph Dawson Pineda on 7/3/25.
//
#include "joint_editor.hpp"
#include "utility.hpp"
namespace ve{
    void JointEditor::BeginFrame() {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();

        // Update save success timer
        if (state.showSaveSuccess) {
            state.saveSuccessTimer -= ImGui::GetIO().DeltaTime;
            if (state.saveSuccessTimer <= 0.0f) {
                state.showSaveSuccess = false;
            }
        }
    }

    void JointEditor::ShowJointEditorWindow(VeGameObject& gameObject, EngineInfo& engineInfo) {
        if (!ImGui::GetCurrentContext()) {
            return;
        }
        if (!gameObject.model) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

        // Set window position and size as requested
        ImGui::SetNextWindowPos(ImVec2(50.0f, io.DisplaySize.y * 0.5f - 300.0f));
        ImGui::SetNextWindowSize(ImVec2(900, 400), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Joint Editor")) {
            ImGui::End();
            return;
        }

        // Check if animation is paused
        bool isPaused = !gameObject.model->animationManager->currentAnimation->isRunning();

        if (!isPaused) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                               "Pause animation to edit joints");
            ImGui::End();
            return;
        }

        // Get skeleton
        auto* skeleton = gameObject.model->skeleton.get();
        if (!skeleton) {
            ImGui::Text("No skeleton available for this model");
            ImGui::End();
            return;
        }

        int numJoints = skeleton->joints.size();

        // Joint selection dropdown
        if (ImGui::BeginCombo("Select Joint",
                              engineInfo.selectedJointIndex >= 0 ?
                              skeleton->joints[state.selectedJoint].name.c_str() :
                              "None")) {

            for (int i = 0; i < numJoints; i++) {
                bool isSelected = (engineInfo.selectedJointIndex == i);
                if (ImGui::Selectable(skeleton->joints[i].name.c_str(), isSelected)) {
                    state.selectedJoint = i;
                    engineInfo.selectedJointIndex = i;
                    InitializeJointEditing(gameObject, i, engineInfo);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // Show controls if a joint is selected and being edited
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
                SaveJointPose(gameObject, state.selectedJoint);
            }

            // Show save success message
            if (state.showSaveSuccess) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Pose saved!");
            }

            if (ImGui::Button("Revert Changes")) {
                RevertJointChanges(gameObject, state.selectedJoint, engineInfo);
            }
        }

        ImGui::End();
    }

    void JointEditor::UpdateJointFromTransform(VeGameObject& gameObject, int jointIndex, const glm::mat4& deltaTransform) {
        auto* skeleton = gameObject.model->skeleton.get();
        if (!skeleton || jointIndex < 0 || jointIndex >= skeleton->joints.size()) {
            return;
        }

        auto& joint = skeleton->joints[jointIndex];

        // ISSUE 1: Add sensitivity scaling for Android
        const float SENSITIVITY_SCALE = 0.1f; // Reduce sensitivity by 90%

        // Extract delta components
        glm::vec3 deltaTranslation, deltaScale, deltaSkew;
        glm::vec4 deltaPerspective;
        glm::quat deltaRotation;
        glm::decompose(deltaTransform, deltaScale, deltaRotation, deltaTranslation, deltaSkew, deltaPerspective);

        // ISSUE 2: Scale down the delta components
        deltaTranslation *= SENSITIVITY_SCALE;

        // For rotation, use slerp to reduce sensitivity
        glm::quat identityQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        deltaRotation = glm::slerp(identityQuat, deltaRotation, SENSITIVITY_SCALE);

        // Get current joint's local transform matrix
        glm::mat4 currentLocalTransform = joint.getLocalMatrix();
        glm::mat4 newLocalTransform;

        if (state.currentOperation == ImGuizmo::TRANSLATE) {
            if (state.currentMode == ImGuizmo::WORLD) {
                // For world mode translation
                if (joint.parentIndex != ve::NO_PARENT) {
                    glm::mat4 parentGlobal = skeleton->jointMatrices[joint.parentIndex];
                    glm::mat4 parentGlobalInv = glm::inverse(parentGlobal);

                    // Convert world delta to local space
                    glm::vec3 localDelta = glm::vec3(parentGlobalInv * glm::vec4(deltaTranslation, 0.0f));
                    joint.translation += localDelta;
                } else {
                    // Root joint - apply delta directly
                    joint.translation += deltaTranslation;
                }
            } else {
                // Local mode - apply delta directly
                joint.translation += deltaTranslation;
            }
        }
        else if (state.currentOperation == ImGuizmo::ROTATE) {
            if (state.currentMode == ImGuizmo::WORLD) {
                // For world mode rotation
                if (joint.parentIndex != ve::NO_PARENT) {
                    // This is more complex - convert world rotation to local space
                    glm::mat4 parentGlobal = skeleton->jointMatrices[joint.parentIndex];
                    glm::quat parentRotation = glm::quat_cast(parentGlobal);
                    glm::quat parentRotationInv = glm::inverse(parentRotation);

                    // Convert world delta rotation to local space
                    glm::quat localDeltaRotation = parentRotationInv * deltaRotation * parentRotation;
                    joint.rotation = localDeltaRotation * joint.rotation;
                } else {
                    // Root joint
                    joint.rotation = deltaRotation * joint.rotation;
                }
            } else {
                // Local mode
                joint.rotation = deltaRotation * joint.rotation;
            }
        }

        // ISSUE 3: Clamp translation to reasonable bounds
        const float MAX_TRANSLATION = 10.0f;
        joint.translation = glm::clamp(joint.translation,
                                       glm::vec3(-MAX_TRANSLATION),
                                       glm::vec3(MAX_TRANSLATION));

        // Update the joint's transform matrices
        skeleton->updateJoint(jointIndex);

        // Update all child joints recursively
        int numJoints = skeleton->joints.size();
        for (int i = 0; i < numJoints; i++) {
            if (skeleton->joints[i].parentIndex == jointIndex) {
                skeleton->updateJoint(i);
            }
        }
    }

    void JointEditor::RenderGizmo(VeGameObject& gameObject, EngineInfo& engineInfo) {
        // Only render if we have a valid joint selected and are editing
        if (state.selectedJoint < 0 || !state.editingJoint) {
            return;
        }

        // Check if animation is paused
        bool isPaused = !gameObject.model->animationManager->currentAnimation->isRunning();
        if (!isPaused) {
            return;
        }

        auto* skeleton = gameObject.model->skeleton.get();
        if (!skeleton) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        auto& joint = skeleton->joints[state.selectedJoint];
        glm::mat4 jointWorldMatrix;

        if (joint.parentIndex != ve::NO_PARENT) {
            jointWorldMatrix = skeleton->jointMatrices[joint.parentIndex] * joint.getLocalMatrix();
        } else {
            jointWorldMatrix = joint.getLocalMatrix();
        }

        // Apply gameObject transform
        glm::mat4 gizmoTransform = gameObject.transform.mat4() * jointWorldMatrix;

        // Store the gizmo transform before manipulation
        glm::mat4 gizmoTransformBefore = gizmoTransform;

        // Add snap settings for more precise control
        float snap[3] = { 0.1f, 0.1f, 0.1f }; // Translation snap
        float rotationSnap = 5.0f; // 5-degree rotation snap

        bool useSnap = ImGui::GetIO().KeyShift; // Hold shift for snapping

        // Manipulate the gizmo
        bool wasManipulated = ImGuizmo::Manipulate(
                glm::value_ptr(engineInfo.camera.getRotViewMatrix()),
                glm::value_ptr(engineInfo.camera.getProjectionMatrix()),
                state.currentOperation,
                state.currentMode,
                glm::value_ptr(gizmoTransform),
                nullptr, // delta matrix (not used)
                useSnap ? (state.currentOperation == ImGuizmo::ROTATE ? &rotationSnap : snap) : nullptr
        );

        // If the gizmo was manipulated, calculate the delta and apply to joint
        if (wasManipulated) {
            // Calculate the delta transformation in a more stable way
            glm::mat4 deltaTransform = gizmoTransform * glm::inverse(gizmoTransformBefore);

            // ISSUE 6: Check for reasonable delta to prevent extreme jumps
            glm::vec3 deltaTranslation = glm::vec3(deltaTransform[3]);
            float deltaLength = glm::length(deltaTranslation);

            // If delta is too large, ignore it (likely a gizmo glitch)
            if (deltaLength < 5.0f) { // Adjust threshold as needed
                UpdateJointFromTransform(gameObject, state.selectedJoint, deltaTransform);
            }
        }
    }

    void JointEditor::InitializeJointEditing(VeGameObject& gameObject, int jointIndex, EngineInfo& engineInfo) {
        auto* skeleton = gameObject.model->skeleton.get();
        if (!skeleton || jointIndex < 0 || jointIndex >= skeleton->joints.size()) {
            state.editingJoint = false;
            return;
        }

        // Check if animation is paused
        bool isPaused = !gameObject.model->animationManager->currentAnimation->isRunning();
        if (!isPaused) {
            state.editingJoint = false;
            return;
        }

        // Store the original joint's local transform values FROM THE CURRENT KEYFRAME
        auto& joint = skeleton->joints[jointIndex];

        // Get the current animation and time to ensure we're storing the keyframe values
        auto* animationManager = gameObject.model->animationManager.get();
        if (animationManager && animationManager->currentAnimation) {
            // Store current time and subtract frame time to compensate for updateAnimation advancing it
            float currentTime = animationManager->currentAnimation->currentKeyFrameTime;
            animationManager->currentAnimation->currentKeyFrameTime = currentTime - engineInfo.frameTime;

            // Force update the animation to ensure joint has correct keyframe values
            // This will advance the time back to where it was originally
            gameObject.updateAnimation(engineInfo.frameTime, engineInfo.frameCount, engineInfo.frameIndex);
        }

        // Now store the original values (these should be the keyframe values)
        state.originalTranslation = joint.translation;
        state.originalRotation = joint.rotation;
        state.originalScale = joint.scale;

        // Get the joint's global transform for reference
        glm::mat4 globalTransform;
        if (skeleton->isAnimated) {
            globalTransform = skeleton->joints[jointIndex].getLocalMatrix();

            // Apply parent transforms if not root
            int16_t parentIndex = skeleton->joints[jointIndex].parentIndex;
            if (parentIndex != ve::NO_PARENT) {
                globalTransform = skeleton->jointMatrices[parentIndex] * globalTransform;
            }
        } else {
            globalTransform = skeleton->joints[jointIndex].jointWorldMatrix;
        }

        // Store original transform and position for reference
        state.originalTransform = globalTransform;
        state.currentTransform = globalTransform;
        state.originalPosition = glm::vec3(globalTransform[3]);
        state.currentEditPosition = state.originalPosition;
        state.editingJoint = true;
    }

    void JointEditor::RevertJointChanges(VeGameObject& gameObject, int jointIndex, EngineInfo& engineInfo) {
        auto* skeleton = gameObject.model->skeleton.get();
        if (!skeleton || jointIndex < 0 || jointIndex >= skeleton->joints.size()) {
            return;
        }

        // Simply restore the original values that were stored when editing began
        auto& joint = skeleton->joints[jointIndex];
        joint.translation = state.originalTranslation;
        joint.rotation = state.originalRotation;
        joint.scale = state.originalScale;

        // Update the joint's transform matrices
        skeleton->updateJoint(jointIndex);

        // Update all child joints recursively
        int numJoints = skeleton->joints.size();
        for (int i = 0; i < numJoints; i++) {
            if (skeleton->joints[i].parentIndex == jointIndex) {
                skeleton->updateJoint(i);
            }
        }

        // Reset the editing state
        state.currentEditPosition = state.originalPosition;
        state.currentTransform = state.originalTransform;
    }


    void JointEditor::SaveJointPose(VeGameObject& gameObject, int jointIndex) {
        auto* skeleton = gameObject.model->skeleton.get();
        if (!skeleton || jointIndex < 0 || jointIndex >= skeleton->joints.size()) {
            return;
        }

        // Get the current animation and time
        auto& animation = *gameObject.model->animationManager->currentAnimation;
        float currentTime = animation.currentKeyFrameTime;

        SaveJointKeyframe(animation, skeleton, jointIndex, currentTime, true, true);

        // Show success message
        state.showSaveSuccess = true;
        state.saveSuccessTimer = 2.0f; // Show for 2 seconds
    }

}