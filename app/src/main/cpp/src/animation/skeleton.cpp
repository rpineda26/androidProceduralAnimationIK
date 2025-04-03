#include "skeleton.hpp"
#include "debug.hpp"
namespace ve{
    Skeleton::Skeleton(){}
    Skeleton::~Skeleton(){}
    void Skeleton::traverse(){
        int indent = 0;
        LOGI("Skeleton indent: %s", name.c_str());
        auto& root = joints[0];
        traverse(root, indent + 1);
    }
    void Skeleton::traverse(Joint const& joint, int indent){
        size_t numChildren = joint.childrenIndices.size();
        for(size_t i = 0; i < numChildren; i++){
            auto& child = joints[joint.childrenIndices[i]];
            LOGI("Skeleton %d: %s", joint.childrenIndices[i], child.name.c_str());
            traverse(child, indent + 1);
        }
    }
    //update the coordinates of all joints based on animation
    void Skeleton::update(){
        int16_t numJoints = static_cast<int16_t>(joints.size());
        // isAnimated = false;
        if(isAnimated){
            //apply animation transform
            for(int16_t i = 0; i < numJoints; i++){
                jointMatrices[i] = joints[i].getAnimatedMatrix();
            }
            //recursively update joint matrices
            updateJoint(ROOT_JOINT);

            //return from animated space to original model space
            for(int16_t i = 0; i < numJoints; i++){
                jointMatrices[i] = jointMatrices[i] * joints[i].inverseBindMatrix;
            }
        }else{
            for(int16_t i=0; i<numJoints;i++){
                jointMatrices[i] = glm::mat4(1.0f);
            }
        }
    }
    void Skeleton::updateJoint(int16_t jointIndex){
        auto& currentJoint = joints[jointIndex];
        int16_t parentJoint = currentJoint.parentIndex;
        if(parentJoint != NO_PARENT){
            jointMatrices[jointIndex] = jointMatrices[parentJoint] * jointMatrices[jointIndex];
        }
        size_t numChildren = currentJoint.childrenIndices.size();
        for(size_t i = 0; i < numChildren; i++){
            updateJoint(currentJoint.childrenIndices[i]);
        }
    }
    bool Skeleton::isDescendantOf(int childIndex, int ancestorIndex) {
        int16_t parent = joints[childIndex].parentIndex;
        while (parent != ve::NO_PARENT) {
            if (parent == ancestorIndex) return true;
            parent = joints[parent].parentIndex;
        }
        return false;
    }
    // Implementation of the new methods
    void Skeleton::updateJointMatrices() {
        // First set local matrices for all joints
        for (int16_t i = 0; i < joints.size(); i++) {
            auto& joint = joints[i];

            // Create the local transformation matrix
            glm::mat4 localTransform = glm::mat4(1.0f);

            // Apply rotation
            glm::mat4 rotationMatrix = glm::mat4_cast(joint.rotation);
            localTransform = localTransform * rotationMatrix;

            // Apply translation
            localTransform = glm::translate(localTransform, joint.translation);

            // Store in jointMatrices
            jointMatrices[i] = localTransform;
        }
    }

    void Skeleton::applyParentTransforms(int16_t jointIndex) {
        auto& currentJoint = joints[jointIndex];
        int16_t parentJoint = currentJoint.parentIndex;

        if (parentJoint != NO_PARENT) {
            jointMatrices[jointIndex] = jointMatrices[parentJoint] * jointMatrices[jointIndex];
        }

        // Recursively process children
        for (size_t i = 0; i < currentJoint.childrenIndices.size(); i++) {
            applyParentTransforms(currentJoint.childrenIndices[i]);
        }
    }
}