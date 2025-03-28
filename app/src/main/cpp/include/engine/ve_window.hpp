//Vulkan engine window header file
#pragma once
//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <vulkan/vulkan.h>
#include <string>
namespace ve {
    class VeWindow {
        public:
            struct ANativeWindowDeleter {
                void operator()(ANativeWindow *new_window) { ANativeWindow_release(new_window); }
            };
            VeWindow(std::string name, ANativeWindow* nativeWindow);
            ~VeWindow();
            VeWindow(const VeWindow &) = delete; 
            VeWindow &operator=(const VeWindow &) = delete;

            VkExtent2D getExtent() { return {static_cast<uint32_t>(ANativeWindow_getWidth(window.get())),
                                             static_cast<uint32_t>(ANativeWindow_getHeight(window.get()))}; }
            ANativeWindow *getWindow() { return window.get(); }
            bool isOrientationChanged() { return orientationChanged; }
            void setOrientationChanged(bool changed) { orientationChanged = changed; }


            void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
            void resetWindow(ANativeWindow *newWindow);
        private:
            std::string windowName;
            std::unique_ptr<ANativeWindow, ANativeWindowDeleter> window;
            bool orientationChanged = false;
    };
}