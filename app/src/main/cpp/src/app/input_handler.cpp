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

            cameraMovement.deltaX = (newX - primaryTouch.x) * (ROTATION_SENSITIVITY * globalSensitivity);
            cameraMovement.deltaY = (newY - primaryTouch.y) * (ROTATION_SENSITIVITY * globalSensitivity);

            // Update primary touch position
            primaryTouch.x = newX;
            primaryTouch.y = newY;
            return true;
        }
        if (event->pointerCount >= 2) {
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

            // Add validation to prevent extreme zoom values
            if (oldDistance > 10.0f && newDistance > 10.0f) { // Minimum distance threshold
                float distanceDelta = newDistance - oldDistance;
                // Clamp the distance delta to prevent extreme jumps
                distanceDelta = glm::clamp(distanceDelta, -100.0f, 100.0f);

                cameraMovement.pinchScale = 1.0f - distanceDelta * (ZOOM_SENSITIVITY * globalSensitivity);
                // Clamp pinch scale to reasonable range
                cameraMovement.pinchScale = glm::clamp(cameraMovement.pinchScale, 0.5f, 2.0f);
            } else {
                // If fingers are too close, don't apply zoom
                cameraMovement.pinchScale = 1.0f;
            }

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
                cameraMovement = {};
                return true;

            case AMOTION_EVENT_ACTION_POINTER_DOWN: // Subsequent pointers
                if (pointerCount == 2) {
                    // Pinch/zoom detection
                    secondaryTouch.x = event->pointers[1].rawX;
                    secondaryTouch.y = event->pointers[1].rawY;
                    secondaryTouch.pointerId = event->pointers[1].id;
                    cameraMovement = {};
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
                    cameraMovement = {};
                    // Update primary touch to current position to avoid jump
                    if (event->pointerCount > 0) {
                        primaryTouch.x = event->pointers[0].rawX;
                        primaryTouch.y = event->pointers[0].rawY;
                    }
                }
                return true;

            default:
                return false;
        }
    }
    void InputHandler::processMovement(VeCamera& camera) {
        if(isModelMode)
            return;
//        LOGI("VeCamera processCameraMovement called");
//        LOGI("deltaX: %f, deltaY: %f, zoom: %f", cameraMovement.deltaX, cameraMovement.deltaY, cameraMovement.pinchScale);
        camera.horizontalAngle += cameraMovement.deltaX * (HORIZONTAL_SENSITIVITY * globalSensitivity);
        camera.verticalAngle += cameraMovement.deltaY * (VERTICAL_SENSITIVITY * globalSensitivity);
        camera.verticalAngle = glm::clamp(camera.verticalAngle, MIN_ELEVATION, MAX_ELEVATION);
        camera.horizontalAngle = fmod(camera.horizontalAngle, 360.0f);

        // Apply zoom with additional safety checks
        if (cameraMovement.pinchScale != 1.0f) {
            float newRadius = camera.radius * cameraMovement.pinchScale;
            // Only apply if the new radius is reasonable
            if (newRadius >= MIN_RADIUS && newRadius <= MAX_RADIUS) {
                camera.radius = newRadius;
            }
        }
//        LOGI("radius at %f", camera.radius);
    }
    void InputHandler::setModelMode(bool enabled) {
        isModelMode = enabled;
        // Reset movement data when switching modes
        cameraMovement = {};
//        LOGI("InputHandler mode switched to: %s", enabled ? "Model" : "Camera");
    }

    bool InputHandler::getModelMode() const {
        return isModelMode;
    }

    void InputHandler::processModelMovement(TransformComponent& transform) {
        if (!isModelMode) return;

//        LOGI("Model processMovement called");
//        LOGI("deltaX: %f, deltaY: %f, pinchScale: %f",
//             cameraMovement.deltaX, cameraMovement.deltaY, cameraMovement.pinchScale);

        // Apply rotation - deltaX controls Y-axis rotation (yaw), deltaY controls X-axis rotation (pitch)
        transform.rotation.y += cameraMovement.deltaX * (MODEL_ROTATION_SENSITIVITY * globalSensitivity);
        transform.rotation.x += cameraMovement.deltaY * (MODEL_ROTATION_SENSITIVITY * globalSensitivity);

        // Optional: Clamp pitch to prevent model flipping upside down
        transform.rotation.x = glm::clamp(transform.rotation.x, glm::radians(-89.0f), glm::radians(89.0f));

        // Keep yaw in 0-2π range for cleaner values
        transform.rotation.y = fmod(transform.rotation.y, glm::two_pi<float>());
        if (transform.rotation.y < 0) transform.rotation.y += glm::two_pi<float>();

        // Apply scaling from pinch gesture - use multiplicative scaling to maintain proportions
        if (cameraMovement.pinchScale != 1.0f) {
            // Use the pinch scale directly but with sensitivity adjustment
            float scaleMultiplier = 1.0f + (cameraMovement.pinchScale - 1.0f) * (MODEL_SCALE_SENSITIVITY * globalSensitivity);

            // Apply uniform scaling to maintain model proportions
            transform.scale *= scaleMultiplier;

            // Clamp each component to prevent extreme scaling
            transform.scale.x = glm::clamp(transform.scale.x, MIN_MODEL_SCALE, MAX_MODEL_SCALE);
            transform.scale.y = glm::clamp(transform.scale.y, MIN_MODEL_SCALE, MAX_MODEL_SCALE);
            transform.scale.z = glm::clamp(transform.scale.z, MIN_MODEL_SCALE, MAX_MODEL_SCALE);
        }

//        LOGI("Model rotation: pitch=%f, yaw=%f, scale=(%f,%f,%f)",
//             glm::degrees(transform.rotation.x), glm::degrees(transform.rotation.y),
//             transform.scale.x, transform.scale.y, transform.scale.z);
    }

    void InputHandler::setGlobalSensitivity(float sensitivity) {
        globalSensitivity = sensitivity;
    }

    float InputHandler::getGlobalSensitivity() const {
        return globalSensitivity;
    }

}
