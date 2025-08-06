#pragma once
#include "skeleton.hpp"
#include <string>
#include <memory>

namespace ve{
    
    class Animation{
        public:
            enum class PathType {
                TRANSLATION,
                ROTATION,
                SCALE
            };
            enum class InterpolationMethod{
                LINEAR,
                STEP
                // CUBICSPLINE
            };
            struct Channel {
                PathType pathType;       // "translation", "rotation", "scale"
                int samplerIndex; 
                int node;
            };
            struct Sampler{
                std::vector<float> timeStamps;
                std::vector<glm::vec4> TRSoutputValues;
                InterpolationMethod interpolationMethod;
            };
            Animation(std::string const& name);
            void start();
            void stop();
            bool isRunning() const;
            bool willExpire(const float& deltaTime )const; 
            void update(const float& deltaTime, Skeleton& skeleton);
            void updatePose(Skeleton& skeleton);
            void setProgress(float progress);
            void setPlayBackSpeed(float speed){playbackSpeed = speed;}
            void setRepeat(){isRepeat = !isRepeat;}
            void setFirstKeyFrameTime(float frameTime){firstKeyFrameTime = frameTime;}
            void setLastKeyFrameTime(float frameTime){lastKeyFrameTime = frameTime;}
            float getDuration() const{return lastKeyFrameTime - firstKeyFrameTime;}
            float getProgress() const{return currentKeyFrameTime / getDuration();}
            float getCurrentTime() const{return currentKeyFrameTime - firstKeyFrameTime;}
            float getLastKeyFrameTime() const{return lastKeyFrameTime;}
            float getFirstKeyFrameTime() const{return firstKeyFrameTime;}

            std::string const& getName() const{return name;}
            bool getIsRepeat()const {return isRepeat;}
            std::vector<float> getKeyframeTimes();

            std::vector<Channel> channels;
            std::vector<Sampler> samplers;
            float currentKeyFrameTime = 0.0f;
            float playbackSpeed = 1.0f;
        private:
            std::string name;
            bool isRepeat;
            bool isPaused{false};
            float firstKeyFrameTime;
            float lastKeyFrameTime;

    };
}