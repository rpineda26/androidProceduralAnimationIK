plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

android {
    namespace = "com.mslabs.pineda.vulkanandroid"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.mslabs.pineda.vulkanandroid"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                arguments(
                    "-DANDROID_TOOLCHAIN=clang",
//                    "-DANDROID_STL=c++_static",  // Change this from c++_static to c++_shared
                    "-DVKB_VALIDATION_LAYERS=OFF"
                )
                abiFilters("arm64-v8a")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    buildFeatures {
        viewBinding = true
        prefab = true
    }

    sourceSets {
        getByName("main") {
            java.srcDirs("src/main/java", "src/main/kotlin")
            res.srcDirs("src/main/res")
            manifest.srcFile("src/main/AndroidManifest.xml")
        }
    }
}

dependencies {

    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.constraintlayout)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)

    //dependencies for Android Game Development source: https://developer.android.com/jetpack/androidx/releases/games
    // To use the Android Frame Pacing library
//    implementation(libs.androidx.games.frame.pacing)
    // To use the Android Performance Tuner
//    implementation(libs.androidx.games.performance.tuner)
    // To use the Games Activity library
    implementation(libs.androidx.games.activity)
    // To use the Games Controller Library
//    implementation(libs.androidx.games.controller)
    // To use the Games Text Input Library
    // Do not include this if games-activity has been included
    // implementation(libs.androidx.games.text.input)
}