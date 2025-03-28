//
// Created by Ralph Pineda on 3/28/25.
//
#include "input_handler.hpp"
#include "debug.hpp"

namespace ve{
    void InputHandler::resetTouchState() {
        primaryTouch={};
        secondaryTouch={};
        cameraMovement={};
    }
    void InputHandler::resetSecondaryTouch() {
        secondaryTouch={};
    }
    bool InputHandler::handleTouchMove(const GameActivityMotionEvent *event) {
        if(!primaryTouch.isTouched)
            return false;
        if(!secondaryTouch.isTouched){
            float newX = event->pointers[0].rawX;
            float newY = event->pointers[0].rawY;

            cameraMovement.deltaX = (newX - primaryTouch.x) * ROTATION_SENSITIVITY;
            cameraMovement.deltaY = (newY - primaryTouch.y) * ROTATION_SENSITIVITY;

            // Update primary touch position
            primaryTouch.x = newX;
            primaryTouch.y = newY;
            return true;
        }
        if (primaryTouch.isTouched && secondaryTouch.isTouched) {
            float oldDistance = std::hypotf(
                    secondaryTouch.x - primaryTouch.x,
                    secondaryTouch.y - primaryTouch.y
            );

            // Update touch positions
            primaryTouch.x = event->pointers[0].rawX;
            primaryTouch.y = event->pointers[0].rawY;
            secondaryTouch.x = event->pointers[1].rawX;
            secondaryTouch.y = event->pointers[1].rawY;

            float newDistance = std::hypotf(
                    secondaryTouch.x - primaryTouch.x,
                    secondaryTouch.y - primaryTouch.y
            );

            // Calculate zoom scale
            cameraMovement.pinchScale = 1.0f - (newDistance - oldDistance) * ZOOM_SENSITIVITY;
            return true;
        }

        return false;
    }
    InputHandler::CameraMovement InputHandler::getCameraMovement() {
        CameraMovement movement = cameraMovement;
        cameraMovement = {};
        return movement;
    }
    bool InputHandler::handleTouchEvent(const GameActivityMotionEvent* event) {
        if (!event) return false;

        int32_t action = event->action;
        size_t pointerCount = event->pointerCount;

        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN: // First pointer down
                primaryTouch.x = event->pointers[0].rawX;
                primaryTouch.y = event->pointers[0].rawY;
                primaryTouch.pointerId = event->pointers[0].id;
                primaryTouch.isTouched = true;
                return true;

            case AMOTION_EVENT_ACTION_POINTER_DOWN: // Subsequent pointers
                if (pointerCount == 2) {
                    // Pinch/zoom detection
                    secondaryTouch.x = event->pointers[1].rawX;
                    secondaryTouch.y = event->pointers[1].rawY;
                    secondaryTouch.pointerId = event->pointers[1].id;
                    secondaryTouch.isTouched = true;
                }
                return true;

            case AMOTION_EVENT_ACTION_MOVE:
                return handleTouchMove(event);

            case AMOTION_EVENT_ACTION_UP: // Last pointer up
                resetTouchState();
                return true;

            case AMOTION_EVENT_ACTION_POINTER_UP:
                // Handle secondary pointer removal
                if (pointerCount == 1) {
                    resetSecondaryTouch();
                }
                return true;

            default:
                return false;
        }
    }
    void InputHandler::processMovement(VeCamera& camera) {
        LOGI("VeCamera processCameraMovement called");
        LOGI("deltaX: %f, deltaY: %f, zoom: %f", cameraMovement.deltaX, cameraMovement.deltaY, cameraMovement.pinchScale);
        camera.horizontalAngle += cameraMovement.deltaX * HORIZONTAL_SENSITIVITY;
        camera.verticalAngle += cameraMovement.deltaY * VERTICAL_SENSITIVITY;
        camera.verticalAngle = glm::clamp(camera.verticalAngle, MIN_ELEVATION, MAX_ELEVATION);
        camera.horizontalAngle = fmod(camera.horizontalAngle, 360.0f);

        camera.radius = glm::clamp(camera.radius * cameraMovement.pinchScale, MIN_RADIUS, MAX_RADIUS);
        LOGI("radius at %f", camera.radius);
    }
}
