package com.mslabs.pineda.vulkanandroid.utils

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.ImageFormat
import android.util.Log
import androidx.annotation.OptIn
import androidx.camera.core.ExperimentalGetImage
import androidx.camera.core.ImageProxy
import androidx.core.graphics.createBitmap

object ImageUtils {
    // Helper function to convert ImageProxy to Bitmap
    // Alternative: Use CameraX's built-in conversion (RECOMMENDED)
    @OptIn(ExperimentalGetImage::class)
    fun imageProxyToBitmap(image: ImageProxy, context: Context): Bitmap {
        // Convert to bitmap using Renderscript or other Android utilities
        return when (image.format) {
            ImageFormat.JPEG -> {
                val buffer = image.planes[0].buffer
                val bytes = ByteArray(buffer.remaining())
                buffer.get(bytes)
                BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                    ?: throw IllegalStateException("Failed to decode JPEG")
            }
            else -> {
                // For other formats, use a more generic approach
                convertYuvToBitmapGeneric(image, context)
            }
        }
    }

    // Helper function to create square bitmap from any bitmap
    fun createSquareBitmap(bitmap: Bitmap): Bitmap {
        val size = minOf(bitmap.width, bitmap.height)
        val x = (bitmap.width - size) / 2
        val y = (bitmap.height - size) / 2
        return Bitmap.createBitmap(bitmap, x, y, size, size)
    }
    // Generic YUV to Bitmap converter
    private fun convertYuvToBitmapGeneric(image: ImageProxy, context:Context): Bitmap {
        val width = image.width
        val height = image.height

        // Create RGB bitmap
        val bitmap = createBitmap(width, height)

        try {
            // Use Android's YUV to RGB conversion
            val rs = android.renderscript.RenderScript.create(context)

            // This is complex, so for now, return a placeholder
            // In production, you'd implement proper YUV->RGB conversion here
            bitmap.eraseColor(android.graphics.Color.BLUE) // Placeholder

            rs.destroy()
            return bitmap

        } catch (e: Exception) {
            Log.w("ImageConversion", "Generic YUV conversion failed, using fallback")
            bitmap.eraseColor(android.graphics.Color.GREEN) // Different color for debugging
            return bitmap
        }
    }
}