package com.mslabs.pineda.vulkanandroid.ml

import android.content.Context
import android.graphics.Bitmap
import android.util.Log
import com.mslabs.pineda.vulkanandroid.models.AnimalDetectionResult
import com.mslabs.pineda.vulkanandroid.models.AnimalLabels
import com.mslabs.pineda.vulkanandroid.models.DogLabels
import com.mslabs.pineda.vulkanandroid.models.Recognition
import com.mslabs.pineda.vulkanandroid.utils.Constants
import org.tensorflow.lite.DataType
import org.tensorflow.lite.support.image.TensorImage
import org.tensorflow.lite.support.tensorbuffer.TensorBuffer
import java.nio.ByteBuffer
import java.nio.ByteOrder
import androidx.core.graphics.scale

class DogClassifier(private val context: Context) {
    fun detectAnimals(bitmap: Bitmap): AnimalDetectionResult {
        return try {

            val model = AnimalMlModel.newInstance(context)

            // Resize bitmap to exactly 224x224
            val resizedBitmap = bitmap.scale(Constants.IMAGE_SIZE, Constants.IMAGE_SIZE)

            // Try using TensorImage for automatic preprocessing if model has metadata
            val inputFeature0 = TensorBuffer.createFixedSize(intArrayOf(1, Constants.IMAGE_SIZE, Constants.IMAGE_SIZE, 3), DataType.FLOAT32)

            // Option 1: Try TensorImage (automatic preprocessing)
            try {
                val tensorImage = TensorImage.fromBitmap(resizedBitmap)
                inputFeature0.loadBuffer(tensorImage.buffer)
                Log.d("AnimalDetection", "Using TensorImage automatic preprocessing")
            } catch (e: Exception) {
                Log.d("AnimalDetection", "TensorImage failed, falling back to manual preprocessing: ${e.message}")
                // Option 2: Fall back to manual EfficientNet preprocessing
                val byteBuffer = convertBitmapToByteBufferEfficientNet(resizedBitmap)
                inputFeature0.loadBuffer(byteBuffer)
            }

            // Run inference
            val outputs = model.process(inputFeature0)
            val outputFeature0 = outputs.outputFeature0AsTensorBuffer
            val confidences = outputFeature0.floatArray

            // Debug output
            Log.d("AnimalDetection", "Model output size: ${confidences.size}")
            Log.d("AnimalDetection", "Labels size: ${AnimalLabels.labels.size}")
            Log.d("AnimalDetection", "Max confidence: ${confidences.maxOrNull()}")
            Log.d("AnimalDetection", "Min confidence: ${confidences.minOrNull()}")
            Log.d("AnimalDetection", "Raw confidences array: ${confidences.joinToString { String.format("%.6f", it) }}")
            Log.d("AnimalDetection", "Top 5 confidences: ${confidences.take(5).joinToString()}")

            // Create results
            val results = mutableListOf<Recognition>()

            if (confidences.isNotEmpty()) {
                // Find top 5 indices by confidence
                val indexedConfidences = confidences.mapIndexed { index, confidence ->
                    index to confidence
                }.sortedByDescending { it.second }

                val maxResults = minOf(5, indexedConfidences.size)

                for (i in 0 until maxResults) {
                    val (modelIndex, confidence) = indexedConfidences[i]

                    // Ensure we don't exceed the labels array bounds
                    val safeIndex = modelIndex.coerceAtMost(AnimalLabels.labels.size - 1)

                    if (safeIndex >= 0 && safeIndex < AnimalLabels.labels.size) {
                        val label = AnimalLabels.labels[safeIndex]
                        results.add(
                            Recognition(
                                id = i.toString(),
                                title = label,
                                confidence = confidence
                            )
                        )
                        Log.d("AnimalDetection", "Result $i: $label (${String.format("%.3f", confidence)})")
                    } else {
                        Log.w("AnimalDetection", "Invalid index: $modelIndex (max: ${AnimalLabels.labels.size - 1})")
                    }
                }
            } else {
                Log.w("AnimalDetection", "Empty confidences array")
            }

            // Check if dog is detected with sufficient confidence
            val dogDetection = results.find { it.title.equals("Dog", ignoreCase = true) }
            val hasDog = dogDetection != null && dogDetection.confidence > Constants.CONFIDENCE_THRESHOLD // Much lower threshold for debugging

            Log.d("AnimalDetection", "Dog detection details:")
            Log.d("AnimalDetection", "Dog found in results: ${dogDetection != null}")
            if (dogDetection != null) {
                Log.d("AnimalDetection", "Dog confidence: ${dogDetection.confidence}")
                Log.d("AnimalDetection", "Dog confidence scientific: ${String.format("%.10f", dogDetection.confidence)}")
            }

            Log.d("AnimalDetection", "Dog detected: $hasDog with confidence: ${dogDetection?.confidence ?: 0f}")

            // Clean up
            model.close()
            if (resizedBitmap != bitmap) {
                resizedBitmap.recycle()
            }

            AnimalDetectionResult(
                hasDog = hasDog,
                confidence = dogDetection?.confidence ?: 0f,
                detectedAnimals = results
            )

        } catch (e: Exception) {
            Log.e("AnimalDetection", "Error in detectAnimals: ${e.message}", e)
            e.printStackTrace()
            AnimalDetectionResult(
                hasDog = false,
                confidence = 0f,
                detectedAnimals = emptyList()
            )
        }
    }
    fun recognizeDogBreed(bitmap: Bitmap, sensorOrientation: Int = 0): List<Recognition> {
        return try {
            val model = Model.newInstance(context)

            // Resize bitmap to exactly 224x224
            val resizedBitmap = bitmap.scale(Constants.IMAGE_SIZE, Constants.IMAGE_SIZE)

            // Prepare input tensor
            val inputFeature0 = TensorBuffer.createFixedSize(intArrayOf(1, Constants.IMAGE_SIZE, Constants.IMAGE_SIZE, 3), DataType.FLOAT32)
            val byteBuffer = convertBitmapToByteBuffer(resizedBitmap)
            inputFeature0.loadBuffer(byteBuffer)

            // Run inference
            val outputs = model.process(inputFeature0)
            val outputFeature0 = outputs.outputFeature0AsTensorBuffer
            val confidences = outputFeature0.floatArray

            // Debug output
            Log.d("Classification", "Model output size: ${confidences.size}")
            Log.d("Classification", "Labels size: ${DogLabels.labels.size}")
            Log.d("Classification", "Max confidence: ${confidences.maxOrNull()}")
            Log.d("Classification", "Top 5 confidences: ${confidences.take(5).joinToString()}")

            // Create results
            val results = mutableListOf<Recognition>()

            if (confidences.isNotEmpty()) {
                // Find top 3 indices by confidence
                val indexedConfidences = confidences.mapIndexed { index, confidence ->
                    index to confidence
                }.sortedByDescending { it.second }

                val maxResults = minOf(3, indexedConfidences.size)

                for (i in 0 until maxResults) {
                    val (modelIndex, confidence) = indexedConfidences[i]

                    // Ensure we don't exceed the labels array bounds
                    val safeIndex = modelIndex.coerceAtMost(DogLabels.labels.size - 1)

                    if (safeIndex >= 0 && safeIndex < DogLabels.labels.size) {
                        val label = DogLabels.labels[safeIndex]
                        results.add(
                            Recognition(
                                id = i.toString(),
                                title = label,
                                confidence = confidence
                            )
                        )
                        Log.d("Classification", "Result $i: $label (${String.format("%.3f", confidence)})")
                    } else {
                        Log.w("Classification", "Invalid index: $modelIndex (max: ${DogLabels.labels.size - 1})")
                    }
                }
            } else {
                Log.w("Classification", "Empty confidences array")
            }

            // Clean up
            model.close()
            if (resizedBitmap != bitmap) {
                resizedBitmap.recycle()
            }

            results

        } catch (e: Exception) {
            Log.e("Classification", "Error in recognizeDogBreed ${e.message}", e)
            e.printStackTrace()
            emptyList()
        }
    }
    // EfficientNet preprocessing function (ImageNet normalization)
    private fun convertBitmapToByteBufferEfficientNet(bitmap: Bitmap): ByteBuffer {
        val byteBuffer = ByteBuffer.allocateDirect(4 * Constants.IMAGE_SIZE * Constants.IMAGE_SIZE * 3)
        byteBuffer.order(ByteOrder.nativeOrder())

        val intValues = IntArray(Constants.IMAGE_SIZE * Constants.IMAGE_SIZE)
        bitmap.getPixels(intValues, 0, bitmap.width, 0, 0, bitmap.width, bitmap.height)

        // ImageNet mean and std for EfficientNet preprocessing
        val mean = floatArrayOf(0.485f, 0.456f, 0.406f) // RGB means
        val std = floatArrayOf(0.229f, 0.224f, 0.225f)  // RGB stds

        var pixel = 0
        for (i in 0 until Constants.IMAGE_SIZE) {
            for (j in 0 until Constants.IMAGE_SIZE) {
                val pixelValue = intValues[pixel++]

                val r = (pixelValue shr 16 and 0xFF)
                val g = (pixelValue shr 8 and 0xFF)
                val b = (pixelValue and 0xFF)

                // EfficientNet preprocessing: normalize to [0,1] then apply ImageNet normalization
                val rNorm = ((r / 255.0f) - mean[0]) / std[0]
                val gNorm = ((g / 255.0f) - mean[1]) / std[1]
                val bNorm = ((b / 255.0f) - mean[2]) / std[2]

                byteBuffer.putFloat(rNorm)
                byteBuffer.putFloat(gNorm)
                byteBuffer.putFloat(bNorm)
            }
        }

        return byteBuffer
    }
    private fun convertBitmapToByteBuffer(bitmap: Bitmap): ByteBuffer {
        val byteBuffer = ByteBuffer.allocateDirect(4 * Constants.IMAGE_SIZE * Constants.IMAGE_SIZE * 3)
        byteBuffer.order(ByteOrder.nativeOrder())

        val intValues = IntArray(Constants.IMAGE_SIZE * Constants.IMAGE_SIZE)
        bitmap.getPixels(intValues, 0, bitmap.width, 0, 0, bitmap.width, bitmap.height)

        var pixel = 0
        for (i in 0 until Constants.IMAGE_SIZE) {
            for (j in 0 until Constants.IMAGE_SIZE) {
                val pixelValue = intValues[pixel++]

                val r = (pixelValue shr 16 and 0xFF)
                val g = (pixelValue shr 8 and 0xFF)
                val b = (pixelValue and 0xFF)

                // TRY THESE NORMALIZATIONS ONE BY ONE:

                // Option 1: [0, 1] normalization (most common)
                byteBuffer.putFloat(r / 255.0f)
                byteBuffer.putFloat(g / 255.0f)
                byteBuffer.putFloat(b / 255.0f)

                // Option 2: [-1, 1] normalization (uncomment to try)
                // byteBuffer.putFloat((r - 127.5f) / 127.5f)
                // byteBuffer.putFloat((g - 127.5f) / 127.5f)
                // byteBuffer.putFloat((b - 127.5f) / 127.5f)

                // Option 3: [0, 255] no normalization (uncomment to try)
                // byteBuffer.putFloat(r.toFloat())
                // byteBuffer.putFloat(g.toFloat())
                // byteBuffer.putFloat(b.toFloat())
            }
        }

        return byteBuffer
    }

}