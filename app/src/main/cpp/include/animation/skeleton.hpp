#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <map>

namespace ve{
    struct Joint {
        std::string name;
        int parentIndex;
        std::vector<int> childrenIndices;
        glm::mat4 jointWorldMatrix{1.0f};
        glm::mat4 inverseBindMatrix;

        //animation TRS matrices
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; 
        glm::vec3 scale{1.0f};
        glm::mat4 getAnimatedMatrix(){
            return    glm::translate(glm::mat4(1.0f), translation) 
                    * glm::mat4(rotation) 
                    * glm::scale(glm::mat4(1.0f), scale)
                    * jointWorldMatrix;
        }
        glm::mat4 getLocalMatrix() const {
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
            glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
            
            return translationMatrix * rotationMatrix * scaleMatrix;
        }
    };
    
    static constexpr int NO_PARENT = -1;
    static constexpr int ROOT_JOINT = 0;

    class Skeleton{
        public:
            Skeleton();
            ~Skeleton();
            void traverse();
            void traverse(Joint const& joint, int indent=0);
            void update();
            void updateJoint(int16_t jointIndex);
            //public for now
            std::string name;
            std::vector<Joint> joints;
            bool isAnimated = true;
            std::vector<glm::mat4> jointMatrices;
            std::map<int, int> nodeJointMap;
    };
}