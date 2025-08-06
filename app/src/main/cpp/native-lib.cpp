/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//#include "engine.h"
#include "first_app.hpp"
#include "debug.hpp"
#include "ve_imgui.hpp"
#include "dog_classifier.hpp"


#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

struct VulkanEngine {
    struct android_app *app;
    ve::FirstApp *app_backend;
    DogClassifier classifier;
    bool canRender = false;
    bool modelLoaded = false;

    // Helper method to load model
    bool loadClassificationModel() {
        if (modelLoaded) {
            return true; // Already loaded
        }

        if (!app || !app->activity || !app->activity->assetManager) {
            LOGE("Cannot load model: missing asset manager");
            return false;
        }

        LOGI("Loading dog breed classification model...");
        bool success = classifier.loadModel(app->activity->assetManager,
                                            "dog_breed_model_rf.xml",
                                            "dog_breed_model_breeds.xml");
        if (success) {
            LOGI("Dog breed model loaded successfully");
            modelLoaded = true;
        } else {
            LOGE("Failed to load dog breed model");
        }
        return success;
    }
};
// The global engine pointer for JNI access.
// It will be managed in sync with app->userData.
static VulkanEngine* g_engine = nullptr;

/**
 * Handles commands from the Android application lifecycle.
 */
static void HandleCmd(struct android_app *app, int32_t cmd) {
    // Retrieve the engine from userData. It might be null.
    auto *engine = (VulkanEngine *)app->userData;

    switch (cmd) {
        // The system is starting and the window might be available.
        // This is the best place to create our engine.
        case APP_CMD_INIT_WINDOW:
            LOGI("Called - APP_CMD_INIT_WINDOW");
            // If the engine doesn't exist, create it.
            if (engine == nullptr) {
                LOGI("Engine is null, creating a new instance.");
                engine = new VulkanEngine();
                engine->app = app;
                engine->app_backend = new ve::FirstApp();

                // Store the new engine in userData so we can access it later.
                app->userData = engine;
                // Sync the global pointer for JNI.
                g_engine = engine;
            }

            // Now, proceed with initialization.
            if (app->window != nullptr) {
                LOGI("Window is ready, initializing backend.");
                engine->app_backend->reset(app->window, app->activity->assetManager);
                engine->app_backend->init();
                engine->canRender = true;
                if (!engine->modelLoaded) {
                    engine->loadClassificationModel();
                }
            }
            break;

            // The window is being destroyed. We must stop rendering.
        case APP_CMD_TERM_WINDOW:
            LOGI("Called - APP_CMD_TERM_WINDOW");
            if (engine) {
                engine->canRender = false;
                // Clean up surface-dependent resources (swapchain).
                // The logical device and other core resources remain.
                if (engine->app_backend) {
                    engine->app_backend->cleanupSurface();
                }
            }
            break;

            // The application is being destroyed. We must release all resources.
        case APP_CMD_DESTROY:
            LOGI("Called - APP_CMD_DESTROY");
            if (engine) {
                LOGI("Destroying engine and backend.");
                engine->canRender = false;
                engine->modelLoaded = false;

                // Fully clean up and delete the backend.
                if (engine->app_backend) {
                    engine->app_backend->cleanup();
                    delete engine->app_backend;
                    engine->app_backend = nullptr;
                }

                // Delete the main engine object itself.
                delete engine;

                // IMPORTANT: Nullify the pointers to prevent stale access.
                app->userData = nullptr;
                g_engine = nullptr;
                engine = nullptr;
            }
            break;

            // Other cases like APP_CMD_START can be removed if their logic
            // is handled by APP_CMD_INIT_WINDOW, or kept for other tasks.
        case APP_CMD_START:
            LOGI("Called - App_CMD_START");
            // This is a good place for logic that doesn't depend on the window.
            break;

        default:
            break;
    }
}


extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_toggleDogModelList(JNIEnv *env, jobject thiz) {
    if(g_engine == nullptr || g_engine->app_backend == nullptr)
        return;
    g_engine->app_backend->toggleDogModelList();
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_toggleEditJointsList(JNIEnv *env, jobject thiz) {
    if(g_engine == nullptr || g_engine->app_backend == nullptr)
        return;
    g_engine->app_backend->toggleEditJointMode();
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_toggleChangeAnimationList(JNIEnv *env, jobject thiz) {
    if(g_engine == nullptr || g_engine->app_backend == nullptr)
        return;
    g_engine->app_backend->toggleChangeAnimationList();
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_toggleXraySkeleton(JNIEnv *env, jobject thiz) {
    if(g_engine == nullptr || g_engine->app_backend == nullptr)
        return;
    g_engine->app_backend->toggleOutlignHighlight();
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_jumpForward(JNIEnv *env, jobject thiz) {
    if(g_engine == nullptr || g_engine->app_backend == nullptr)
        return;
    g_engine->app_backend->jumpForward();
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_jumpBackward(JNIEnv *env, jobject thiz) {
    if(g_engine == nullptr || g_engine->app_backend == nullptr)
        return;
    g_engine->app_backend->jumpBackward();
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_togglePlayPauseAnimation(JNIEnv *env, jobject thiz, jboolean value) {
    if (g_engine == nullptr || g_engine->app_backend == nullptr)
        return JNI_FALSE;

    bool shouldPlay = (value == JNI_TRUE);

    // Ask C++ to try to set it; C++ returns true if playing, false if paused.
    bool isPlaying = g_engine->app_backend->toggleAnimationPlayer(shouldPlay);

    return isPlaying ? JNI_TRUE : JNI_FALSE;
}
extern "C" JNIEXPORT jfloat JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_getAnimationDurationFromBackend(JNIEnv* env, jobject thiz) {
    if (g_engine == nullptr || g_engine->app_backend == nullptr) return 0.0f;
    return g_engine->app_backend->getAnimationDuration();
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_pauseAnimation(JNIEnv* env, jobject thiz) {
    if (g_engine == nullptr || g_engine->app_backend == nullptr) return;

    try {
        g_engine->app_backend->pauseAnimation();
    } catch (...) {
        // Ignore errors
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_setModelMode(JNIEnv* env, jobject thiz, jboolean value) {
    if (g_engine == nullptr || g_engine->app_backend == nullptr) return;

    try {
        bool shouldModelMode = (value == JNI_TRUE);
        g_engine->app_backend->setModelMode(shouldModelMode);
    } catch (...) {
        // Ignore errors
    }
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_isAnimationPlaying(JNIEnv* env, jobject thiz) {
    if (g_engine == nullptr || g_engine->app_backend == nullptr) return JNI_FALSE;

    try {
        return g_engine->app_backend->isAnimationPlaying() ? JNI_TRUE : JNI_FALSE;
    } catch (...) {
        return JNI_FALSE;
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_resumeAnimation(JNIEnv* env, jobject thiz) {
    if (g_engine == nullptr || g_engine->app_backend == nullptr) return;

    try {
        g_engine->app_backend->resumeAnimation();
    } catch (...) {
        // Ignore errors
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_setAnimationTime(JNIEnv* env, jobject thiz, jfloat time) {
    if (g_engine == nullptr || g_engine->app_backend == nullptr) return;
    g_engine->app_backend->setAnimationTime(time);
}
extern "C" JNIEXPORT jfloat JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_getCurrentAnimationTime(JNIEnv* env, jobject thiz) {
    if (g_engine == nullptr || g_engine->app_backend == nullptr) return 0.0f;
    return g_engine->app_backend->getCurrentAnimationTime();
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_nativeOnPhotoCaptured(JNIEnv *env, jobject thiz, jstring modelName)  {
    LOGI("SUCCESS: Photo captured! Bridge between utils_NativeInterface and C++ backend is working!");

    try {
        const char *modelNameChars = env->GetStringUTFChars(modelName, nullptr);
        std::string modelNameStr(modelNameChars);
        env->ReleaseStringUTFChars(modelName, modelNameChars);
        // Use the random number in your function call
        std::string r2 = g_engine->app_backend->changeModel(modelNameStr);


    } catch (...) {
        LOGI("Error during classification");
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_mslabs_pineda_vulkanandroid_utils_NativeInterface_setSensitivity(JNIEnv* env, jobject thiz, jfloat sensitivity) {
    LOGI("SUCCESS: Photo captured! Bridge between utils_NativeInterface and C++ backend is working!");

    try {
        g_engine->app_backend->inputHandler.setGlobalSensitivity(sensitivity);

    } catch (...) {
        LOGI("Error during classification");
    }
}

/*
 * Process user touch and key events. GameActivity double buffers those events,
 * applications can process at any time. All of the buffered events have been
 * reported "handled" to OS. For details, refer to:
 * d.android.com/games/agdk/game-activity/get-started#handle-events
 */
static void HandleInputEvents(struct android_app *app) {
    auto inputBuf = android_app_swap_input_buffers(app);
    if (inputBuf == nullptr) {
        return;
    }
//    LOGI("INPUT EVENT OCCURED");
    auto *engine = (VulkanEngine *)app->userData;
    if(engine->app_backend->isInitialized()) {

        for (int i = 0; i < inputBuf->motionEventsCount; i++) {
            auto event = &inputBuf->motionEvents[i];
            if(!ve::VeImGui::handleInput(event)) {
                engine->app_backend->inputHandler.handleTouchEvent(event);
                engine->app_backend->controlCamera();
            }
        }
    }
    android_app_clear_motion_events(inputBuf);
}

/**
 * Main entry point for a native Android application.
 */
void android_main(struct android_app *state) {
    // Set the command handler and clear userData initially.
    state->onAppCmd = HandleCmd;
    state->userData = nullptr; // Start with a clean slate.

    // Optional: Set input filters if you handle input.
    // android_app_set_key_event_filter(state, ...);
    // android_app_set_motion_event_filter(state, ...);

    LOGI("Starting main event loop.");

    while (true) {
        int ident;
        int events;
        android_poll_source *source;

        // The ALooper_pollOnce call will block until an event occurs,
        // or return immediately if rendering is enabled.
        // We check g_engine as an extra safeguard.
        int timeout = (g_engine && g_engine->canRender) ? 0 : -1;

        if ((ident = ALooper_pollOnce(timeout, nullptr, &events, (void **)&source)) >= 0) {
            // Process this event. This will call HandleCmd.
            if (source != nullptr) {
                source->process(state, source);
            }

            // If the app is being destroyed, HandleCmd will have cleaned up
            // and set destroyRequested. We should exit the loop.
            if (state->destroyRequested != 0) {
                LOGI("Destroy requested, exiting main loop.");
                return;
            }
        }

        // If we can render, run the main loop logic.
        if (g_engine && g_engine->canRender) {
            HandleInputEvents(state); // Process any pending input
            g_engine->app_backend->run(); // Render a frame
        }
    }
}