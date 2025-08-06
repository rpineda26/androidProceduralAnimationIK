//
// Created by Ralph Pineda on 3/28/25.
//

#ifndef VULKANANDROID_INPUT_HANDLER_HPP
#define VULKANANDROID_INPUT_HANDLER_HPP
#include "ve_game_object.hpp"
#include "ve_camera.hpp"
#include <android/input.h>
#include <glm/glm.hpp>
#include <cmath>
#include <game-activity/native_app_glue/android_native_app_glue.h>

namespace ve{
    class InputHandler{
    public:
        struct TouchState{
            float x = 0.0f;
            float y = 0.0f;
            int32_t pointerId = -1;
            bool isTouched = false;
        };
        struct CameraMovement{
            float deltaX = 0.0f;
            float deltaY = 0.0f;
            float pinchScale = 1.0; //zoom
        };
    public:
        InputHandler() = default;
        bool handleTouchEvent(const GameActivityMotionEvent *event);
        CameraMovement getCameraMovement();
        void processMovement(VeCamera& camera);
        // New methods for model mode
        void setModelMode(bool enabled);
        bool getModelMode() const;
        void processModelMovement(TransformComponent& transform);

        //sensitivity control
        void setGlobalSensitivity(float sensitivity);
        float getGlobalSensitivity() const ;

    private:
        bool handleTouchMove(const GameActivityMotionEvent *event);
        void resetTouchState();
        void resetSecondaryTouch();

    private:
        TouchState primaryTouch;
        TouchState secondaryTouch;
        CameraMovement cameraMovement;

        // Add model mode state
        bool isModelMode = false;

        //api sensitivity parameter
        float globalSensitivity = 1.0f;

        //camera sensitivity settings
        const float ROTATION_SENSITIVITY = 0.5f;
        const float ZOOM_SENSITIVITY = 0.01f;
        const float HORIZONTAL_SENSITIVITY = 0.5f;
        const float VERTICAL_SENSITIVITY = 0.5f;

        // Model control sensitivity
        const float MODEL_ROTATION_SENSITIVITY = 0.1f;  // Reduced from 0.5f
        const float MODEL_SCALE_SENSITIVITY = 0.5f;     // Increased from 0.01f
        const float MIN_MODEL_SCALE = 0.1f;
        const float MAX_MODEL_SCALE = 5.0f;
    };
}
#endif //VULKANANDROID_INPUT_HANDLER_HPP

