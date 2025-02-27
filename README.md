# androidProceduralAnimationIK
android renderer using vulkan api

## Prerequisites

* Android SDK 26 (Android 8.0) or higher
* Android NDK r21 or newer
* CMake 3.10.2+
* JDK 11+

Android NDK and CMake can be installed via Android Studio:

1. Open **Android Studio**  
2. Go to **Tools** -> **SDK Manager**  
3. Navigate to **Android SDK** -> **SDK Tools**  
4. Check the required tools (e.g., CMake, NDK)  
5. Click **Apply** to install

**Set up environment variables**
   ```bash
   # For macOS/Linux
    export ANDROID_HOME=~/path/to/android/sdk
    export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/[version]
    export PATH=$PATH:$ANDROID_HOME/cmake/[version]/bin
    # Optional: Set up paths if using your own CMake, or build with the command line using platform tools, or build tools
    
    export PATH=$PATH:$ANDROID_HOME/platform-tools
    export PATH=$PATH:$ANDROID_HOME/build-tools/[version]
   ```

## Setup and Run with Android Studio
1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/your-vulkan-project.git
   cd your-vulkan-project
   ```

2. **Open in Android Studio**
   * Launch Android Studio
   * Select "Open an Existing Project"
   * Navigate to the cloned repository directory and open it

3. **Sync Project**
   * Android Studio should automatically sync the Gradle files
   * If not, select "File" > "Sync Project with Gradle Files"

4. **Configure NDK and CMake**
   * Open "File" > "Project Structure" > "SDK Location"
   * Ensure NDK and CMake paths are correctly set
   * If not installed, use the SDK Manager to download them

5. **Build the Project**
   * Select "Build" > "Make Project" or press Ctrl+F9 (Cmd+F9 on Mac)

6. **Run on Device/Emulator**
   * Connect a physical device via USB (enable USB debugging) or setup an emulator with Vulkan support
   * Select "Run" > "Run 'app'" or press Shift+F10 (Ctrl+R on Mac)
   * Choose your target device from the device selection dialog

## Setup and Run via Command Line

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/your-vulkan-project.git
   cd your-vulkan-project
   ```

3. **Build the project**
   ```bash
   # On Windows
   gradlew.bat assembleDebug
   
   # On macOS/Linux
   ./gradlew assembleDebug
   ```

4. **Install on device**
   ```bash
   # Make sure a device is connected and recognized via ADB
   adb devices
   
   # Install the debug APK
   adb install app/build/outputs/apk/debug/app-debug.apk
   ```

5. **Run the application**
   ```bash
   # Get the package name from your AndroidManifest.xml
   # For example, if your package is com.example.vulkanapp:
   adb shell am start -n com.mslabs.pineda.vulkanandroid/.MainActivity
   ```

## Troubleshooting

### Common Issues

1. **Vulkan not supported on device**
   * Ensure your test device supports Vulkan
   * Check with: `adb shell getprop ro.hardware.vulkan`

2. **NDK build failures**
   * Verify that you're using a compatible NDK version
   * Check CMake logs for detailed error information

3. **Failed to find CMake/NDK**
   * Ensure paths are correctly set in local.properties or environment variables
