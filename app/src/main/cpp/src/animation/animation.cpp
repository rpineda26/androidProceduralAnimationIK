#include "animation.hpp"
#include "debug.hpp"

#include <imgui.h>
#include <vector>
#include <set>
#include <iostream>
namespace ve{
    Animation::Animation(std::string const& name): name(name), isRepeat(true){}
    void Animation::start(){
        if(currentKeyFrameTime==0.0f && currentKeyFrameTime > lastKeyFrameTime)
            currentKeyFrameTime = firstKeyFrameTime;
        isPaused = false;
    }
    void Animation::stop(){
        isPaused = true;
    }
    bool Animation::isRunning() const{
        return (isRepeat || (currentKeyFrameTime <= lastKeyFrameTime)) && !isPaused;
    }
    bool Animation::willExpire(const float& deltaTime) const{
        return !isRepeat || (currentKeyFrameTime + deltaTime > lastKeyFrameTime);
    }
    void Animation::update(const float& deltaTime, ve::Skeleton& skeleton){
        if(!isRunning()){
//            LOGE("Attempting to update Animation %s while it is not running", name.c_str());
            return;
        }
        float adjustedDeltaTime = deltaTime * playbackSpeed;

        // Ensure a minimum impact
        if (playbackSpeed > 1.0f) {
            // For speeds > 1, scale up more dramatically
            adjustedDeltaTime *= playbackSpeed;
        } else if (playbackSpeed < 1.0f) {
            // For speeds < 1, ensure some progression
            adjustedDeltaTime = std::max(adjustedDeltaTime, deltaTime * 0.1f);
        }

        currentKeyFrameTime += adjustedDeltaTime;

        if(isRepeat && (currentKeyFrameTime > lastKeyFrameTime)){
            currentKeyFrameTime = firstKeyFrameTime;
        }
        
        // std::cout << "Current key frame time: " << currentKeyFrameTime << std::endl;
        for(auto& channel: channels){
            auto& sampler = samplers[channel.samplerIndex];
            int jointIndex = skeleton.nodeJointMap[channel.node];
            auto& joint = skeleton.joints[jointIndex];

            bool keyFrameFound = false;
            for(size_t i = 0; i < sampler.timeStamps.size() - 1; i++){
                if(currentKeyFrameTime>=sampler.timeStamps[i] && currentKeyFrameTime <= sampler.timeStamps[i+1]){
                    keyFrameFound = true;
//                    LOGI("Found keyframe interval %f to %f", sampler.timeStamps[i], sampler.timeStamps[i+1]);

                    switch (sampler.interpolationMethod){
                        case InterpolationMethod::LINEAR:{                 
                            float t = (currentKeyFrameTime - sampler.timeStamps[i]) / (sampler.timeStamps[i + 1] - sampler.timeStamps[i]);
                            switch (channel.pathType){
                                case PathType::TRANSLATION:
                                    joint.translation = glm::mix(sampler.TRSoutputValues[i], sampler.TRSoutputValues[i+1], t);
                                    break;
                                case PathType::ROTATION:
                                    glm::quat q1;
                                    q1.x = sampler.TRSoutputValues[i].x;
                                    q1.y = sampler.TRSoutputValues[i].y;
                                    q1.z = sampler.TRSoutputValues[i].z;
                                    q1.w = sampler.TRSoutputValues[i].w;
                                    glm::quat q2;
                                    q2.x = sampler.TRSoutputValues[i+1].x;
                                    q2.y = sampler.TRSoutputValues[i+1].y;
                                    q2.z = sampler.TRSoutputValues[i+1].z;
                                    q2.w = sampler.TRSoutputValues[i+1].w;
                                    // Check quaternions for validity
                                    joint.rotation = glm::normalize(glm::slerp(q1, q2, t));
                                    break;
                                case PathType::SCALE:
                                    joint.scale = glm::mix(sampler.TRSoutputValues[i], sampler.TRSoutputValues[i+1], t);
                                    break;
                            }
                            break;
                        }
                        case InterpolationMethod::STEP:{
                            switch (channel.pathType){
                                case PathType::TRANSLATION:{
                                    joint.translation = glm::vec3(sampler.TRSoutputValues[i]);
                                    break;
                                }
                                case PathType::ROTATION:{
                                    glm::quat q;
                                    q.x = sampler.TRSoutputValues[i].x;
                                    q.y = sampler.TRSoutputValues[i].y;
                                    q.z = sampler.TRSoutputValues[i].z;
                                    q.w = sampler.TRSoutputValues[i].w;

                                    joint.rotation = q;
                                    break;
                                }
                                case PathType::SCALE:{

                                    joint.scale = glm::vec3(sampler.TRSoutputValues[i]);
                                    break;
                                }
                                default:
                                    LOGE("Unknown channel path type: %d", static_cast<int>(channel.pathType));
                                    break;
                            }
                            break;
                        }
                        default:
                            LOGE("Unknown interpolation method: %d", static_cast<int>(sampler.interpolationMethod));
                            break;
                            
                    }
                }
            }
            if (!keyFrameFound) {
                std::string timeStamps = "";
                for (size_t i = 0; i < sampler.timeStamps.size(); i++) {
                    timeStamps += std::to_string(sampler.timeStamps[i]) + " ";
                }
                LOGE("No keyframe found for current time %f", currentKeyFrameTime);
//                LOGI("Available keyframe times: %s", timeStamps.c_str());

            }

        }
//        LOGI("Animation %s updated", name.c_str());
    }
    void Animation::updatePose(Skeleton& skeleton){

        if(isRepeat && (currentKeyFrameTime > lastKeyFrameTime)){
            currentKeyFrameTime = firstKeyFrameTime;
        }

        // std::cout << "Current key frame time: " << currentKeyFrameTime << std::endl;
        for(auto& channel: channels){
            auto& sampler = samplers[channel.samplerIndex];
            int jointIndex = skeleton.nodeJointMap[channel.node];
            auto& joint = skeleton.joints[jointIndex];

            bool keyFrameFound = false;
            for(size_t i = 0; i < sampler.timeStamps.size() - 1; i++){
                if(currentKeyFrameTime>=sampler.timeStamps[i] && currentKeyFrameTime <= sampler.timeStamps[i+1]){
                    keyFrameFound = true;
//                    LOGI("Found keyframe interval %f to %f", sampler.timeStamps[i], sampler.timeStamps[i+1]);

                    switch (sampler.interpolationMethod){
                        case InterpolationMethod::LINEAR:{
                            float t = (currentKeyFrameTime - sampler.timeStamps[i]) / (sampler.timeStamps[i + 1] - sampler.timeStamps[i]);
                            switch (channel.pathType){
                                case PathType::TRANSLATION:
                                    joint.translation = glm::mix(sampler.TRSoutputValues[i], sampler.TRSoutputValues[i+1], t);
                                    break;
                                case PathType::ROTATION:
                                    glm::quat q1;
                                    q1.x = sampler.TRSoutputValues[i].x;
                                    q1.y = sampler.TRSoutputValues[i].y;
                                    q1.z = sampler.TRSoutputValues[i].z;
                                    q1.w = sampler.TRSoutputValues[i].w;
                                    glm::quat q2;
                                    q2.x = sampler.TRSoutputValues[i+1].x;
                                    q2.y = sampler.TRSoutputValues[i+1].y;
                                    q2.z = sampler.TRSoutputValues[i+1].z;
                                    q2.w = sampler.TRSoutputValues[i+1].w;
                                    // Check quaternions for validity
                                    joint.rotation = glm::normalize(glm::slerp(q1, q2, t));
                                    break;
                                case PathType::SCALE:
                                    joint.scale = glm::mix(sampler.TRSoutputValues[i], sampler.TRSoutputValues[i+1], t);
                                    break;
                            }
                            break;
                        }
                        case InterpolationMethod::STEP:{
                            switch (channel.pathType){
                                case PathType::TRANSLATION:{
                                    joint.translation = glm::vec3(sampler.TRSoutputValues[i]);
                                    break;
                                }
                                case PathType::ROTATION:{
                                    glm::quat q;
                                    q.x = sampler.TRSoutputValues[i].x;
                                    q.y = sampler.TRSoutputValues[i].y;
                                    q.z = sampler.TRSoutputValues[i].z;
                                    q.w = sampler.TRSoutputValues[i].w;

                                    joint.rotation = q;
                                    break;
                                }
                                case PathType::SCALE:{

                                    joint.scale = glm::vec3(sampler.TRSoutputValues[i]);
                                    break;
                                }
                                default:
                                    LOGE("Unknown channel path type: %d", static_cast<int>(channel.pathType));
                                    break;
                            }
                            break;
                        }
                        default:
                            LOGE("Unknown interpolation method: %d", static_cast<int>(sampler.interpolationMethod));
                            break;

                    }
                }
            }
            if (!keyFrameFound) {
                std::string timeStamps = "";
                for (size_t i = 0; i < sampler.timeStamps.size(); i++) {
                    timeStamps += std::to_string(sampler.timeStamps[i]) + " ";
                }
                LOGE("No keyframe found for current time %f", currentKeyFrameTime);
//                LOGI("Available keyframe times: %s", timeStamps.c_str());

            }

        }
//        LOGI("Animation %s updated", name.c_str());
    }
    void Animation::setProgress(float progress){
        progress =  std::clamp(progress, 0.0f, getDuration());
        // Calculate absolute time based on total duration
        currentKeyFrameTime = progress;
    }
    std::vector<float> Animation::getKeyframeTimes(){
        std::set<float> uniqueTimes;
        for (const ve::Animation::Sampler& sampler : samplers) {
            // 4. Iterate through the timeStamps vector in the current sampler
            for (float time : sampler.timeStamps) {
                // 5. Insert each timestamp into the set.
                //    Duplicates will be automatically ignored by the set.
                uniqueTimes.insert(time);
            }
        }

        // std::vector sorts the timeframes in order
        std::vector<float> keyframeTimesVec(uniqueTimes.begin(), uniqueTimes.end());
        return keyframeTimesVec;
    }
}