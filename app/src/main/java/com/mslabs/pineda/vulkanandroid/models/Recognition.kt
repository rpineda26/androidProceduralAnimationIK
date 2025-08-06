package com.mslabs.pineda.vulkanandroid.models

data class Recognition(
    val id: String,
    val title: String,
    val confidence: Float
)
data class AnimalDetectionResult(
    val hasDog: Boolean,
    val confidence: Float,
    val detectedAnimals: List<Recognition>
)
