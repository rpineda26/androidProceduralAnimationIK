package com.mslabs.pineda.vulkanandroid

import android.annotation.SuppressLint
import android.os.Build.VERSION
import android.os.Build.VERSION_CODES
import android.view.KeyEvent
import android.view.View
import android.view.WindowManager.LayoutParams
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import android.os.Bundle
import com.mslabs.pineda.vulkanandroid.databinding.ActivityMainBinding
import com.google.androidgamesdk.GameActivity;

class MainActivity : GameActivity() {

    private lateinit var binding: ActivityMainBinding

        override fun onCreate(savedInstanceState: Bundle?) {
            super.onCreate(savedInstanceState)
            hideSystemUI()

            // Example of a call to a native method
        }

        /**
         * A native method that is implemented by the 'vulkanandroid' native library,
         * which is packaged with this application.
         */
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
    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        var processed = super.onKeyDown(keyCode, event);
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            onBackPressed()
            processed = true
        }
        return processed
    }

    companion object {
        // Used to load the 'vulkanandroid' library on application startup.
        init {
            System.loadLibrary("vulkanandroid")
        }
    }
}