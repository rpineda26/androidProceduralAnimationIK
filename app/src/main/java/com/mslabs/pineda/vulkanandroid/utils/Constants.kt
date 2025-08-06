package com.mslabs.pineda.vulkanandroid.utils

import android.Manifest
import androidx.core.graphics.toColorInt

object Constants {
    const val CAMERA_PERMISSION_REQUEST_CODE = 100
    val REQUIRED_PERMISSIONS = arrayOf(
        Manifest.permission.CAMERA
    )
    const val ACTIVE_COLOR = "#FF9933"
    //time interval when jumping forward/backward in animation time
    const val UPDATE_INTERVAL_MS = 16L
    //for the ml models, they are trained on square images 224*224
    const val IMAGE_SIZE = 224
    //for classification, accept result if greater than threshhold
    const val CONFIDENCE_THRESHOLD = 0.001f
}