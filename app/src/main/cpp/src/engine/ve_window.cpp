//Vulkan engine window cpp file
#include "ve_window.hpp"
#include <stdexcept>
namespace ve {
    VeWindow::VeWindow(std::string name, ANativeWindow *nativeWindow) : windowName{name}, window{nativeWindow}{
    }
    VeWindow::~VeWindow() {
        if (window) {
            ANativeWindow_release(window.get());
        }
    }

    void VeWindow::resetWindow(ANativeWindow* newWindow){
        window.reset(newWindow);
    }

    void VeWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
        const VkAndroidSurfaceCreateInfoKHR create_info{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .window = window.get()
        };
        if (vkCreateAndroidSurfaceKHR(instance, &create_info, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

} 