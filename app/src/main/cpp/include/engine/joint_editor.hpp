//
// Created by Ralph Dawson Pineda on 7/3/25.
//

#ifndef VULKANANDROID_JOINT_EDITOR_HPP
#define VULKANANDROID_JOINT_EDITOR_HPP
//engine
#include "ve_game_object.hpp"
#include "frame_info.hpp"
//glm
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"
//imgui
#include "imgui.h"
#include "ImGuizmo.h"
#include "ImSequencer.h"

namespace ve{

    struct JointEditorState {
        int selectedJoint = -1;  // Moved from engine to here
        glm::vec3 originalPosition{0.0f};
        glm::mat4 originalTransform{1.0f};
        glm::mat4 currentTransform{1.0f};
        bool editingJoint = false;
        ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE currentMode = ImGuizmo::WORLD;
        bool showSaveSuccess = false;
        float saveSuccessTimer = 0.0f;
        glm::vec3 originalTranslation{0.0f};
        glm::quat originalRotation{1.0f, 0.0f, 0.0f, 0.0f};  // Identity quaternion
        glm::vec3 originalScale{1.0f};
        glm::vec3 currentEditPosition{0.0f};

        // Reset state when needed
        void reset() {
            selectedJoint = -1;
            editingJoint = false;
            showSaveSuccess = false;
            saveSuccessTimer = 0.0f;
            originalPosition = glm::vec3(0.0f);
            originalTransform = glm::mat4(1.0f);
            currentTransform = glm::mat4(1.0f);
            originalTranslation = glm::vec3(0.0f);
            originalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity quaternion
            originalScale = glm::vec3(1.0f);
            currentEditPosition = glm::vec3(0.0f);
            currentOperation = ImGuizmo::TRANSLATE;
            currentMode = ImGuizmo::WORLD;

        }
    };
    class JointEditor{
    private:

    public:
        JointEditorState state;
        // Constructor
        JointEditor() = default;

        // Destructor
        ~JointEditor() = default;

        // Non-copyable (if you want)
        JointEditor(const JointEditor&) = delete;
        JointEditor& operator=(const JointEditor&) = delete;

        // Movable
        JointEditor(JointEditor&&) = default;
        JointEditor& operator=(JointEditor&&) = default;

        // Initialize ImGuizmo for the frame
        void BeginFrame();

        // Show the joint selection and control window (call from JNI)
        void ShowJointEditorWindow(VeGameObject& gameObject, EngineInfo& engineInfo);

        // Render the 3D gizmo (call in main render loop)
        void RenderGizmo(VeGameObject& gameObject, EngineInfo& engineInfo);

        // Update joint transform from gizmo manipulation
        void UpdateJointFromTransform(VeGameObject& gameObject, int jointIndex, const glm::mat4& transform);

        // Initialize joint editing when a joint is selected
        void InitializeJointEditing(VeGameObject& gameObject, int jointIndex, EngineInfo& engineInfo);

        // Revert joint to original state
        void RevertJointChanges(VeGameObject& gameObject, int jointIndex, EngineInfo& engineInfo);

        // Save current joint pose to animation
        void SaveJointPose(VeGameObject& gameObject, int jointIndex);

        // Reset editor state (useful when switching models/scenes)
        void Reset() { state.reset(); }



    };
}
#endif //VULKANANDROID_JOINT_EDITOR_HPP

