#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

namespace ve{
    static constexpr float MIN_ELEVATION = 1.0f;
    static constexpr float MAX_ELEVATION = 179.0f;
    static constexpr float MIN_RADIUS = 5.0f;
    static constexpr float MAX_RADIUS = 40.0f;
    class VeCamera{
        public:
            VeCamera() = default;
            //methods
            void setOrtho(float left, float right, float bottom, float top, float near, float far);
            void setPerspective(float fovy, float aspect, float near, float far);
            void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{0.0f, 1.0f, 0.0f});
            void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, 1.0f, 0.0f});
            void setViewYXZ(glm::vec3 position, glm::vec3 rotation);
            void calculatePreRotationMatrix(VkSurfaceTransformFlagBitsKHR preTransform);
            //getters
            const glm::mat4& getProjectionMatrix() const { return projectionMatrix; }
            const glm::mat4& getViewMatrix() const { return viewMatrix; }
            const glm::mat4& getInverseMatrix() const { return inverseMatrix; }
            const glm::vec3 getPosition() const { return glm::vec3(inverseMatrix[3]); }
            const glm::mat4& getRotViewMatrix() const { return rotViewMatrix; }
            void getOrbitViewMatrix(glm::vec3 target);

        public:
            float radius{25.0f};
            float horizontalAngle{0.0f};
            float verticalAngle{1.0f};

        private:
            glm::mat4 projectionMatrix{1.0f};
            glm::mat4 viewMatrix{1.0f};
            glm::mat4 inverseMatrix{1.0f};
            glm::mat4 rotViewMatrix{1.0f};



    };
}