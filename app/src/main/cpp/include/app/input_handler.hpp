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
        void processMovement(VeCamera& camera) ;

    private:
        bool handleTouchMove(const GameActivityMotionEvent *event);
        void resetTouchState();
        void resetSecondaryTouch();

    private:
        TouchState primaryTouch;
        TouchState secondaryTouch;
        CameraMovement cameraMovement;

        //sensitivity settings
        const float ROTATION_SENSITIVITY = 0.5f;
        const float ZOOM_SENSITIVITY = 0.01f;
        const float HORIZONTAL_SENSITIVITY = 0.5f;
        const float VERTICAL_SENSITIVITY = 0.5f;
    };
}
#endif //VULKANANDROID_INPUT_HANDLER_HPP
