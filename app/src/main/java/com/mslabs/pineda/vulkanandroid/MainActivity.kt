package com.mslabs.pineda.vulkanandroid

import android.annotation.SuppressLint
import android.content.pm.PackageManager
import android.os.Build.VERSION
import android.os.Build.VERSION_CODES
import android.view.View
import android.view.WindowManager.LayoutParams
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.LayoutInflater
import android.view.ViewGroup
import android.widget.ImageButton
import android.widget.SeekBar
import android.widget.TextView
import android.widget.Toast
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageCapture
import androidx.camera.core.ImageCaptureException
import androidx.camera.core.ImageProxy

import androidx.camera.view.CameraController
import androidx.camera.view.LifecycleCameraController
import androidx.camera.view.PreviewView
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.google.androidgamesdk.GameActivity
import androidx.core.graphics.toColorInt
import com.mslabs.pineda.vulkanandroid.ml.DogClassifier
import com.mslabs.pineda.vulkanandroid.ml.ModelMapper
import com.mslabs.pineda.vulkanandroid.utils.Constants

import com.mslabs.pineda.vulkanandroid.utils.ImageUtils
import com.mslabs.pineda.vulkanandroid.utils.NativeInterface
import androidx.core.view.isVisible

class MainActivity : GameActivity() {

    companion object {

        // Used to load the 'vulkanandroid' library on application startup.
        init {
            System.loadLibrary("vulkanandroid")
        }

    }
    private lateinit var cameraController: LifecycleCameraController
    private lateinit var previewView: PreviewView
    private lateinit var animBar: SeekBar
    private lateinit var sensitivityBar: SeekBar

    private lateinit var cameraButton: ImageButton
    private lateinit var captureButton: ImageButton
    private lateinit var dropdownMenu: ImageButton
    private lateinit var changeModelBtn: ImageButton
    private lateinit var viewAngleBtn: ImageButton
    private lateinit var transformBtn: ImageButton
    private lateinit var editBtn: ImageButton
    private lateinit var animBtn: ImageButton
    private lateinit var playPauseBtn: ImageButton
    private lateinit var xrayBtn: ImageButton
    private lateinit var forwardBtn: ImageButton
    private lateinit var backwardBtn: ImageButton
    private lateinit var helpBtn : ImageButton

    private lateinit var cameraTextView: TextView
    private lateinit var menuTextView: TextView
    private lateinit var xrayTextView: TextView
    private lateinit var selectDogTextView: TextView
    private lateinit var viewTextView: TextView
    private lateinit var trsTextView: TextView
    private lateinit var jointsTextView: TextView
    private lateinit var animationsTextView: TextView
    private lateinit var sensitivityTextView: TextView

    //utils
    private lateinit var classifier: DogClassifier
    private lateinit var nativeInterface: NativeInterface

    //app state variables
    private var isPreviewVisible = false
    private var isMenuVisible = true
    private var isDogListVisible = false
    private var isEditJoints = false
    private var isChangeAnimation = false
    private var isPlaying = true
    private var isXray = true
    private var isUserScrubbing = false
    private var wasAnimationPlayingBeforeScrub = true
    private var isHelpVisible = false
    private var activeRadio: ImageButton? = null
    private var activeRadioImGUI: String? = null
    private val activeColor = Constants.ACTIVE_COLOR.toColorInt()
    private var animationDuration = 0f
    private var updateHandler = Handler(Looper.getMainLooper())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        hideSystemUI()
        window.addFlags(LayoutParams.FLAG_KEEP_SCREEN_ON)
        addContentView(
            LayoutInflater.from(this).inflate(R.layout.activity_main, null),
            ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
        )
        setupTextViews()
        setupButtonListeners()
        classifier = DogClassifier(this)
        nativeInterface = NativeInterface()
        previewView = findViewById(R.id.camera_preview)
        // Check permissions first
        if (allPermissionsGranted()) {
            setupCameraController()
        } else {
            requestPermissions()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraController.unbind()
    }

    private fun setupCameraController() {
        cameraController = LifecycleCameraController(this).apply {
            bindToLifecycle(this@MainActivity)
            cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA
            setEnabledUseCases(CameraController.IMAGE_CAPTURE)
        }
    }
    private fun setupTextViews(){
        cameraTextView = findViewById(R.id.cameraTV)
        menuTextView = findViewById(R.id.menuTV)
        xrayTextView = findViewById(R.id.xrayTV)
        selectDogTextView = findViewById(R.id.selectDogTV)
        viewTextView = findViewById(R.id.viewTV)
        trsTextView = findViewById(R.id.trsTV)
        jointsTextView = findViewById(R.id.jointsTV)
        animationsTextView = findViewById(R.id.animationsTV)
        sensitivityTextView = findViewById(R.id.sensitivityTV)
    }
    private fun setupButtonListeners() {
        // Bind the views
        cameraButton = findViewById(R.id.btn_camera_toggle)
        captureButton = findViewById(R.id.btn_camera_capture)
        dropdownMenu = findViewById(R.id.dropdown_menu)
        changeModelBtn = findViewById(R.id.change_model_btn)
        viewAngleBtn = findViewById(R.id.view_angle_btn)
        transformBtn = findViewById(R.id.transform_btn)
        editBtn = findViewById(R.id.edit_btn)
        animBtn = findViewById(R.id.animations_btn)
        playPauseBtn = findViewById(R.id.playPause_btn)
        xrayBtn = findViewById(R.id.xray_btn)
        forwardBtn = findViewById(R.id.forward_btn)
        backwardBtn = findViewById(R.id.backward_btn)
        animBar = findViewById(R.id.seekBar)
        animBar.thumb.setTint(Constants.ACTIVE_COLOR.toColorInt())
        sensitivityBar = findViewById(R.id.sensitivity_slider)
        sensitivityBar.thumb.setTint(Constants.ACTIVE_COLOR.toColorInt())
        helpBtn = findViewById(R.id.help_btn)

        // Set click listeners
        dropdownMenu.setOnClickListener { toggleMenu() }
        cameraButton.setOnClickListener { toggleCameraPreview() }
        captureButton.setOnClickListener { capturePhoto() }
        changeModelBtn.setOnClickListener { setRadioImGuiActive("dogModel") }
        viewAngleBtn.setOnClickListener { nativeInterface.setModelMode(false); setRadioActive(viewAngleBtn); }
        transformBtn.setOnClickListener { nativeInterface.setModelMode(true); setRadioActive(transformBtn); }
        editBtn.setOnClickListener { setRadioImGuiActive("editJoints") }
        animBtn.setOnClickListener { setRadioImGuiActive("animation") }
        playPauseBtn.setOnClickListener { togglePlayPause() }
        xrayBtn.setOnClickListener { toggleXray() }
        forwardBtn.setOnClickListener { nativeInterface.jumpForward() }
        backwardBtn.setOnClickListener { nativeInterface.jumpBackward() }
        sensitivityBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val sensitivity = 0.25f + (progress / 100f)
                nativeInterface.setSensitivity(sensitivity)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
        helpBtn.setOnClickListener {toggleTextViews()}

        // Initial states
        setRadioActive(viewAngleBtn)
        xrayBtn.setBackgroundColor(activeColor)
        setupSeekBarListener()
    }


    private fun toggleTextViews(){
        if(isHelpVisible){
            cameraTextView.visibility = View.GONE
            menuTextView.visibility = View.GONE
            xrayTextView.visibility = View.GONE
            selectDogTextView.visibility = View.GONE
            viewTextView.visibility = View.GONE
            trsTextView.visibility = View.GONE
            jointsTextView.visibility = View.GONE
            animationsTextView.visibility = View.GONE
            sensitivityTextView.visibility = View.GONE
            isHelpVisible = false
            helpBtn.background=null
        }else{
            //show menu buttons
            if(!isMenuVisible)
                toggleMenu()
            //display button texts
            cameraTextView.visibility = View.VISIBLE
            menuTextView.visibility = View.VISIBLE
            xrayTextView.visibility = View.VISIBLE
            selectDogTextView.visibility = View.VISIBLE
            viewTextView.visibility = View.VISIBLE
            trsTextView.visibility = View.VISIBLE
            jointsTextView.visibility = View.VISIBLE
            animationsTextView.visibility = View.VISIBLE
            sensitivityTextView.visibility = View.VISIBLE

            isHelpVisible = true
            helpBtn.setBackgroundColor(activeColor)
        }

    }
    private fun toggleCameraPreview() {
        if (!allPermissionsGranted()) {
            requestPermissions()
            return
        }
        //validation just in case, though this is  unlikely to happen
        if (cameraController == null) {
            setupCameraController()
        }
        isPreviewVisible = !isPreviewVisible
        if (isPreviewVisible) {
            showPreview()
        } else {
            hidePreview()
        }
        // Show/hide capture button based on preview visibility
        captureButton.visibility = if (isPreviewVisible) View.VISIBLE else View.GONE
    }
    private fun showPreview() {
        // Connect controller to preview
        previewView.controller = cameraController
        previewView.visibility = View.VISIBLE

        // Enable both preview and capture
        cameraController.setEnabledUseCases(
            CameraController.IMAGE_CAPTURE or CameraController.VIDEO_CAPTURE
        )
    }
    private fun hidePreview() {
        previewView.visibility = View.GONE
        previewView.controller = null
        // Disable preview, keep only capture
        cameraController.setEnabledUseCases(CameraController.IMAGE_CAPTURE)
    }
    private fun toggleMenu() {
        if (isMenuVisible) {
            // Hide the buttons
            changeModelBtn.visibility = View.GONE
            viewAngleBtn.visibility = View.GONE
            transformBtn.visibility = View.GONE
            xrayBtn.visibility = View.GONE
            editBtn.visibility = View.GONE
            animBtn.visibility = View.GONE
            sensitivityBar.visibility = View.GONE
        } else {
            // Show the buttons
            changeModelBtn.visibility = View.VISIBLE
            viewAngleBtn.visibility = View.VISIBLE
            transformBtn.visibility = View.VISIBLE
            xrayBtn.visibility = View.VISIBLE
            editBtn.visibility = View.VISIBLE
            animBtn.visibility = View.VISIBLE
            sensitivityBar.visibility = View.VISIBLE
        }
        // Flip the state
        isMenuVisible = !isMenuVisible
        if(isMenuVisible)
            dropdownMenu.setBackgroundColor(activeColor)
        else {
            activeRadioImGUI?.let { setRadioImGuiActive(it) }
            dropdownMenu.background = null
        }
    }

    private fun toggleDogList() {
        isDogListVisible = !isDogListVisible
        nativeInterface.toggleDogModelList()
        if(isDogListVisible)
            changeModelBtn.setBackgroundColor(activeColor)
        else
            changeModelBtn.background = null
    }
    private fun toggleViewPlaybackControls(){
        playPauseBtn.visibility = if (playPauseBtn.isVisible) View.GONE else View.VISIBLE
        forwardBtn.visibility = if (forwardBtn.isVisible) View.GONE else View.VISIBLE
        backwardBtn.visibility = if (backwardBtn.isVisible) View.GONE else View.VISIBLE
        animBar.visibility = if (animBar.isVisible) View.GONE else View.VISIBLE
    }
    private fun toggleEditJoints(){
        isEditJoints = !isEditJoints
        toggleViewPlaybackControls()
        nativeInterface.toggleEditJointsList()
        if (isEditJoints){
            editBtn.setBackgroundColor(activeColor)
        }
        else
            editBtn.background = null
    }
    private fun toggleChangeAnimation(){
        isChangeAnimation = !isChangeAnimation
        nativeInterface.toggleChangeAnimationList()
        if (isChangeAnimation){
            animBtn.setBackgroundColor(activeColor)
        }
        else
            animBtn.background = null
    }
    private fun setRadioImGuiActive(selected: String) {
        // Toggle off current active (if any)
        when (activeRadioImGUI) {
            "dogModel" -> toggleDogList()
            "editJoints" -> toggleEditJoints()
            "animation" -> toggleChangeAnimation()
        }

        // Update state (null if same radio clicked, otherwise set to selected)
        activeRadioImGUI = if (activeRadioImGUI == selected) null else selected

        // Toggle on new active (if any)
        when (activeRadioImGUI) {
            "dogModel" -> toggleDogList()
            "editJoints" -> toggleEditJoints()
            "animation" -> toggleChangeAnimation()
        }
    }

    private fun setRadioActive(selected: ImageButton) {
        if (activeRadio != null && activeRadio != selected) {
            activeRadio?.background = null
        }
        activeRadio = selected
        activeRadio?.setBackgroundColor(activeColor)
    }
    private fun togglePlayPause() {
        val desiredState = !isPlaying
        isPlaying = nativeInterface.togglePlayPauseAnimation(desiredState)
        if (isPlaying) {
            playPauseBtn.setImageResource(R.drawable.pause_24px)
        } else {
            playPauseBtn.setImageResource(R.drawable.play_arrow_24px)
        }
    }
    private fun toggleXray(){
        isXray = !isXray
        nativeInterface.toggleXraySkeleton()
        if(isXray)
            xrayBtn.setBackgroundColor(activeColor)
        else
            xrayBtn.background = null
    }
    private fun setupSeekBarListener(){
        animBar.progress = 0
        animBar.max = 100 // Default, will be updated when animation loads

        animBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                if (fromUser && tryLoadAnimation()) {
                    val newTime = progress / 1000.0f
                    nativeInterface.setAnimationTime(newTime)
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {
                isUserScrubbing = true
                wasAnimationPlayingBeforeScrub = nativeInterface.isAnimationPlaying()
                nativeInterface.pauseAnimation()
            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {
                isUserScrubbing = false
                if (wasAnimationPlayingBeforeScrub) {
                    nativeInterface.resumeAnimation()
                }
            }
        })

        startUpdates()
    }
    private fun tryLoadAnimation(): Boolean {
        if (animationDuration <= 0) {
            try {
                animationDuration = nativeInterface.getAnimationDurationFromBackend()
                if (animationDuration > 0) {
                    animBar.max = (animationDuration * 1000).toInt()
                }
            } catch (e: Exception) {
                return false
            }
        }
        return animationDuration > 0
    }

    private fun startUpdates() {
        updateHandler.postDelayed(object : Runnable {
            override fun run() {
                if (!isUserScrubbing && tryLoadAnimation()) {
                    try {
                        val currentTime = nativeInterface.getCurrentAnimationTime()
                        animBar.progress = (currentTime * 1000).toInt()
                    } catch (e: Exception) {
                        // Ignore errors
                    }
                }
                updateHandler.postDelayed(this, Constants.UPDATE_INTERVAL_MS)
            }
        }, Constants.UPDATE_INTERVAL_MS)
    }

    private fun capturePhoto() {
        Log.d("MainActivity", "capturePhoto() called")

        if (!allPermissionsGranted()) {
            requestPermissions()
            return
        }

        val controller = cameraController
        controller.takePicture(
            ContextCompat.getMainExecutor(this),
            object : ImageCapture.OnImageCapturedCallback() {
                override fun onCaptureSuccess(image: ImageProxy) {
                    try {
                        Log.d("MainActivity", "Image captured: ${image.width}x${image.height}")

                        // Convert to bitmap
                        val bitmap = ImageUtils.imageProxyToBitmap(image, this@MainActivity)
                        Log.d("MainActivity", "Bitmap created: ${bitmap.width}x${bitmap.height}")

                        // Make square if needed
                        val squareBitmap = if (bitmap.width != bitmap.height) {
                            ImageUtils.createSquareBitmap(bitmap)
                        } else {
                            bitmap
                        }

                        Log.d("MainActivity", "Starting classification...")
                        val animalDetectionResult = classifier.detectAnimals(squareBitmap)
                        if (animalDetectionResult.hasDog||true) {
                            Log.d("MainActivity", "Dog detected! Starting breed classification...")

                            // Only run dog breed classification if a dog is detected
                            val results = classifier.recognizeDogBreed(squareBitmap)

                            // Handle breed classification results
                            if (results.isNotEmpty()) {
                                val topResult = results[0]
                                val confidence = (topResult.confidence * 100).toInt()
                                val message = "${topResult.title}: ${confidence}%"

                                Log.d("MainActivity", "Classification result: $message")

                                // Check if we have a 3D model for this breed
                                val modelName = ModelMapper.getModelForLabel(topResult.title)

                                runOnUiThread {
                                    if (modelName != null) {
                                        Toast.makeText(this@MainActivity, "$message - Model available!", Toast.LENGTH_LONG).show()
                                        Log.d("MainActivity", "3D model available for: $modelName")

                                        // Pass the model name to C++
                                        nativeInterface.nativeOnPhotoCaptured(modelName)
                                    } else {
                                        Toast.makeText(this@MainActivity, "$message - No 3D model available", Toast.LENGTH_LONG).show()
                                        Log.w("MainActivity", "No 3D model available for: ${topResult.title}")

                                        // Still call native function but with empty string or handle differently
                                        nativeInterface.nativeOnPhotoCaptured("")
                                    }
                                }
                            } else {
                                Log.w("MainActivity", "No classification results")
                                runOnUiThread {
                                    Toast.makeText(this@MainActivity, "No breed detected", Toast.LENGTH_SHORT).show()
                                }
                            }
                            toggleCameraPreview()

                        } else {
                            Log.d("MainActivity", "No dog detected in the image")

                            // Show what animals were detected instead
                            runOnUiThread {
                                if (animalDetectionResult.detectedAnimals.isNotEmpty()) {
                                    val topAnimal = animalDetectionResult.detectedAnimals[0]
                                    Toast.makeText(this@MainActivity, "Detected: ${topAnimal.title} (not a dog)", Toast.LENGTH_SHORT).show()
                                } else {
                                    Toast.makeText(this@MainActivity, "No animals detected in the image", Toast.LENGTH_SHORT).show()
                                }
                            }
                        }
                        // Clean up bitmaps
                        if (squareBitmap != bitmap) {
                            bitmap.recycle()
                        }
                        squareBitmap.recycle()

                    } catch (e: Exception) {
                        Log.e("MainActivity", "Classification failed: ${e.message}", e)
                        e.printStackTrace()
                        runOnUiThread {
                            Toast.makeText(this@MainActivity, "Error: ${e.message}", Toast.LENGTH_LONG).show()
                        }
                    } finally {
                        image.close()
                    }
                }

                override fun onError(exception: ImageCaptureException) {
                    Log.e("MainActivity", "Photo capture failed: ${exception.message}", exception)
                }
            }
        )
    }

    private fun allPermissionsGranted() = Constants.REQUIRED_PERMISSIONS.all {
        ContextCompat.checkSelfPermission(baseContext, it) == PackageManager.PERMISSION_GRANTED
    }

    private fun requestPermissions() {
        ActivityCompat.requestPermissions(
            this,
            Constants.REQUIRED_PERMISSIONS,
            Constants.CAMERA_PERMISSION_REQUEST_CODE
        )
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        if (requestCode == Constants.CAMERA_PERMISSION_REQUEST_CODE) {
            if (allPermissionsGranted()) {
                setupCameraController()
                Log.d("Permissions", "Camera permissions granted")
            } else {
                Log.e("Permissions", "Camera permissions denied")
                showPermissionDeniedMessage()
            }
        }
    }

    private fun showPermissionDeniedMessage() {
        runOnUiThread {
            Toast.makeText(
                this,
                "Camera permission is required for dog breed detection",
                Toast.LENGTH_LONG
            ).show()
        }
    }

    private fun hideSystemUI() {
        // This will put the game behind any cutouts and waterfalls on devices which have
        // them, so the corresponding insets will be non-zero.

        // We cannot guarantee that AndroidManifest won't be tweaked
        // and we don't want to crash if that happens so we suppress warning.
        @SuppressLint("ObsoleteSdkInt")
        if (VERSION.SDK_INT >= VERSION_CODES.P) {
            if (VERSION.SDK_INT >= VERSION_CODES.R) {
                window.attributes.layoutInDisplayCutoutMode =
                    LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
            }
        }
        val decorView: View = window.decorView
        val controller = WindowInsetsControllerCompat(
            window,
            decorView
        )
        controller.hide(WindowInsetsCompat.Type.systemBars())
        controller.hide(WindowInsetsCompat.Type.displayCutout())
        controller.systemBarsBehavior =
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
    }
}