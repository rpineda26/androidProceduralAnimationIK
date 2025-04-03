#include "animation_manager.hpp"
#include "debug.hpp"

namespace ve{
    AnimationManager::AnimationManager(): currentAnimation(nullptr), frameCount(0){}
    Animation& AnimationManager::operator[](std::string const& animation){
        return *animationsMap[animation];
    }
    Animation& AnimationManager::operator[](int index){
        return *animationsVector[index];
    }
    void AnimationManager::push(pAnimation const& animation){
        if(animation){
            animationsMap.emplace(animation->getName(), animation);
            animationsVector.push_back(animation);
            nameIndexMap[animation->getName()] = animationsVector.size() - 1;
        }else{
            LOGE("Animation is null");
        }
    }
    void AnimationManager::start(std::string const& animation){
        Animation* currAnim= animationsMap[animation].get();
        if(currAnim){
            currentAnimation = currAnim;
            currentAnimation->start();
        }
    }
    void AnimationManager::start(size_t index){
        if(!(index < animationsVector.size())){
            LOGE("Animation start index out of range");
            return;
        }
        Animation* currAnim = animationsVector[index].get();
        if(currAnim){
            currentAnimation = currAnim;
            currentAnimation->start();
        }
    }
    void AnimationManager::stop(){
        if(currentAnimation){
            currentAnimation->stop();
        }
    }
    void AnimationManager::setProgress(std::string const& animation, float progress){
        animationsMap[animation]->setProgress(progress);
    }
    void AnimationManager::setPlaybackSpeed(float speed){
        for(auto& anim:animationsVector){

            float limit = std::clamp(speed, 0.1f, 3.0f);
            anim->setPlayBackSpeed(limit);
        }
    }
    void AnimationManager::update(const float& deltaTime, ve::Skeleton& skeleton, int frameCounter){
        if(frameCount != frameCounter){
            frameCount = frameCounter;
            if(currentAnimation){
                // std::cout<<"updating "<<currentAnimation->getName()<<std::endl;
                currentAnimation->update(deltaTime, skeleton);
            }
        }
    }
    void AnimationManager::updateTimeSkip(ve::Skeleton &skeleton) {
        if(currentAnimation){
            currentAnimation->updatePose(skeleton);
        }else{
            LOGE("Cant update pose, current animation is null");
        }
    }
    bool AnimationManager::isRunning() const{
        if(currentAnimation){
            return currentAnimation->isRunning();
        }
        return false;
    }
    bool AnimationManager::willExpire(const float& deltaTime) const{
        if(currentAnimation){
            return currentAnimation->willExpire(deltaTime);
        }
        return false;
    }
    void AnimationManager::setRepeat(){
        if(currentAnimation){
            currentAnimation->setRepeat();
        }
    }
    void AnimationManager::setRepeatAll(bool repeat){
        for(auto& animation: animationsVector){
            animation->setRepeat();
        }
    }
    float AnimationManager::getCurrentTime(){
        if(currentAnimation){
            return currentAnimation->getCurrentTime();
        }
        return 0.0f;
    }
    float AnimationManager::getDuration(std::string const& animation){
        return animationsMap[animation]->getDuration();
    }
    float AnimationManager::getProgress(std::string const& animation){
        return animationsMap[animation]->getProgress();
    }
    std::string AnimationManager::getName(){
        if(currentAnimation){
            return currentAnimation->getName();
        }
        return "";
    }
    std::vector<std::string> AnimationManager::getAllNames() {
        std::vector<std::string> names;
        for(auto& element : animationsVector){
            names.push_back(element->getName());
        }
        return names;
    }
    int AnimationManager::getIndex(std::string const& animation){
        bool found = false;
        for (auto& element : animationsVector){
            if (element->getName() == animation){
                found = true;
                break;
            }
        }
        if(found)
            return nameIndexMap[animation];
        return -1;
    }

    AnimationManager::Iterator AnimationManager::begin(){
        return Iterator(&(*animationsVector.begin()));
    }
    AnimationManager::Iterator AnimationManager::end(){
        return Iterator(&(*animationsVector.end()));
    }
}