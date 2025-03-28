#pragma once
#include "animation.hpp"
#include <map>
#include <memory>

namespace ve{
    class AnimationManager{
        public: 
            using pAnimation = std::shared_ptr<Animation>;
            struct Iterator{
                Iterator(pAnimation* ptr): ptr(ptr){}
                Iterator& operator++(){
                    ptr++;
                    return *this;
                }
                Animation& operator*(){
                    return **ptr;
                }
                bool operator!=(const Iterator& other){
                    return ptr != other.ptr;
                }
                private:
                    pAnimation* ptr;
            };

        public:
            Iterator begin();
            Iterator end();
            Animation& operator[](std::string const& animation);
            Animation& operator[](int index);

        public:
            AnimationManager();
            size_t size() const {return animationsMap.size();}
            void push(pAnimation const& animation);
            void start(std::string const& animation);
            void start(size_t index);
            void start(){start(0);};
            void stop();
            void update(const float& deltaTime, Skeleton& skeleton, int frameCounter);
            bool isRunning() const;
            bool willExpire(const float& deltaTime) const;
            void setRepeat();
            void setRepeatAll(bool repeat);
            void setProgress(std::string const& animation, float progress);
            void setPlaybackSpeed(float speed);
            float getDuration(std::string const& animation);
            float getProgress(std::string const& animation);
            float getCurrentTime();
            std::vector<std::string> getAllNames();
            std::string getName();
            size_t getNumAvailableAnimations()const{return animationsMap.size();};
            int getIndex(std::string const& animation);
            bool isCurrentAnimRepeat()const {return currentAnimation->getIsRepeat();}
            Animation* currentAnimation;
        private:
            std::map<std::string, std::shared_ptr<Animation>> animationsMap;
            std::vector<std::shared_ptr<Animation>> animationsVector;

            int frameCount;
            std::map<std::string, int> nameIndexMap;
    };
}