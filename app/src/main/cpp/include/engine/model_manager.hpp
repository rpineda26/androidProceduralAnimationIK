//
// Created by Ralph Dawson Pineda on 6/19/25.
//

#ifndef VULKANANDROID_MODEL_MANAGER_HPP
#define VULKANANDROID_MODEL_MANAGER_HPP

#include "ve_device.hpp"
#include "ve_model.hpp"

#include <android/asset_manager.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <list>
#include <future>
#include <mutex>

namespace ve{
    class ModelManager{
    public:
        static constexpr size_t MAX_LOADED_MODELS = 5;

        ModelManager(VeDevice& device, AAssetManager* assetManager);
        ~ModelManager() = default;

        // Main interface
        std::shared_ptr<VeModel> getModel(const std::string& name);
        bool isModelReady(const std::string& name) const;
        bool isModelLoading(const std::string& name) const;
        void initializeModels(VeDevice& device, AAssetManager* assetManager);

        // Initialization
        void preloadEssentials();
        void preloadAsync(const std::vector<std::string>& models);

        // Utility
        const std::vector<std::string>& getAvailableModels() const;
        void clearAll();

        void startAsyncLoad(const std::string& name);

    private:
        VeDevice& device_;
        AAssetManager* assetManager_;
        std::unique_ptr<VeDescriptorPool> modelDescriptorPool{};

        // Cache (LRU)
        std::unordered_map<std::string, std::shared_ptr<VeModel>> cache_;
        std::list<std::string> accessOrder_;

        // Async loading
        std::unordered_map<std::string, std::future<std::shared_ptr<VeModel>>> loading_;
        mutable std::mutex mutex_;

        // Helper methods
        void addToCache(const std::string& name, std::shared_ptr<VeModel> model);
        void evictOldest();
        std::shared_ptr<VeModel> loadModel(const std::string& name);

        std::string getModelPath(const std::string& name) const;


    };
}
#endif //VULKANANDROID_MODEL_MANAGER_HPP
