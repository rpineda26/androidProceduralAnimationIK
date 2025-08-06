# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

project("vulkanandroid")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)
#sdafsd
find_package(game-activity REQUIRED CONFIG)
find_package(OpenCV REQUIRED COMPONENTS OpenCV::opencv_java4)
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u \
    Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#sadfgsfsd
include(FetchContent)

FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)

FetchContent_MakeAvailable(glm)

set(ExternalDir ${CMAKE_CURRENT_SOURCE_DIR}/external)

# Gather all source files in the `src` directory (your imported C++ files)
file(GLOB_RECURSE SRC_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
# Gather ImGui source files
file(GLOB IMGUI_SOURCES
        ${ExternalDir}/imgui/*.cpp
)

file(GLOB_RECURSE SRC_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/include/*)
set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include) # Ensure src/ itself is included

foreach(dir ${SRC_SUBDIRS})
    if (IS_DIRECTORY ${dir})
        list(APPEND INCLUDE_DIRS ${dir})
    endif()
endforeach()

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
)
add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${SRC_FILES}
        ${IMGUI_SOURCES}
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${INCLUDE_DIRS}
        ${ExternalDir}/stb_image
        ${ExternalDir}/tinyobj
        ${ExternalDir}/imgui
        ${ExternalDir}/tinygltf
)

# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC
        # List libraries link to the target library
#        opencv_core
#        opencv_imgproc
        OpenCV::opencv_java4
        glm::glm
        vulkan
        #games-frame-pacing::swappy_static
        #games-performance-tuner::tuningfork_static
        game-activity::game-activity_static
        android
        log)

