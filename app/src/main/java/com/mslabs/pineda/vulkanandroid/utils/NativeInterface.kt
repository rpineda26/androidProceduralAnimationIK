package com.mslabs.pineda.vulkanandroid.utils

class NativeInterface {
    external fun nativeOnPhotoCaptured(modelName: String)
    external fun toggleDogModelList()
    external fun toggleEditJointsList()
    external fun toggleChangeAnimationList()
    external fun toggleXraySkeleton()
    external fun togglePlayPauseAnimation(value: Boolean): Boolean
    external fun jumpForward()
    external fun jumpBackward()
    external fun setAnimationTime(time: Float)
    external fun getAnimationDurationFromBackend(): Float
    external fun getCurrentAnimationTime(): Float
    external fun pauseAnimation()
    external fun resumeAnimation()
    external fun isAnimationPlaying(): Boolean
    external fun setModelMode(value: Boolean)
    external fun setSensitivity(sensitivity: Float)
}