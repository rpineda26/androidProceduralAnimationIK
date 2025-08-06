//
// Created by Ralph Dawson Pineda on 6/19/25.
//
#include "model_manager.hpp"
#include "debug.hpp"
#include <thread>

namespace ve{
    static const std::unordered_map<std::string, std::string> MODEL_PATHS = {
            {"Akita Inu", "models/akita/akita.gltf"},
            {"Beagle", "models/beagle/beagle.gltf"},
            {"Border Collie", "models/border_collie/border_collie.gltf"},
            {"Boxer", "models/boxer/boxer.gltf"},
            {"Bulldog", "models/bulldog/bulldog.gltf"},
            {"Corgi", "models/corgi/corgi.gltf"},
            {"Dalmatian", "models/dalmatian/dalmatian.gltf"},
            {"Doberman", "models/doberman/doberman.gltf"},
            {"French Bulldog", "models/french_bulldog/french_bulldog.gltf"},
            {"Golden Retriever", "models/golden_retriever/golden_retriever.gltf"},
            {"Husky", "models/husky/husky.gltf"},
            {"Jack Russell Terrier", "models/jack_russell_terrier/jack_russell_terrier.gltf"},
            {"Labrador", "models/labrador/labrador.gltf"},
            {"Pitbull", "models/pitbull/pitbull.gltf"},
            {"Pomeranian Spitz", "models/pomeranian_spitz/pomeranian_spitz.gltf"},
            {"Pug", "models/pug/pug.gltf"},
            {"Rottweiler", "models/rottweiler/rottweiler.gltf"},
            {"Shepherd", "models/shepherd/shepherd.gltf"},
            {"Shiba Inu", "models/shiba_inu/shiba_inu.gltf"},
            {"Toy Terrier", "models/toy_terrier/toy_terrier.gltf"},
    };
    // Available model names
    static const std::vector<std::string> AVAILABLE_MODELS = {
            "Akita Inu",
            "Beagle",
            "Border Collie",
            "Boxer",
            "Bulldog",
            "Corgi",
            "Dalmatian",
            "Doberman",
            "French Bulldog",
            "Golden Retriever",
            "Husky",
            "Jack Russell Terrier",
            "Labrador",
            "Pitbull",
            "Pomeranian Spitz",
            "Pug",
            "Rottweiler",
            "Shepherd",
            "Shiba Inu",
            "Toy Terrier"
    };

    // Global instance
    std::unique_ptr<ModelManager> g_modelManager = nullptr;

    ModelManager::ModelManager(VeDevice& device, AAssetManager* assetManager)
            : device_(device), assetManager_(assetManager) {
        LOGI("Creating descriptorr pool for model textures");
        modelDescriptorPool = VeDescriptorPool::Builder(device)
                .setMaxSets(20000)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
                .build();
    }

    std::shared_ptr<VeModel> ModelManager::getModel(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if already loaded
        auto it = cache_.find(name);
        if (it != cache_.end()) {
            // Move to front (most recently used)
            accessOrder_.remove(name);
            accessOrder_.push_front(name);
            return it->second;
        }

        // Check if loading
        auto loadingIt = loading_.find(name);
        if (loadingIt != loading_.end()) {
            // Check if finished loading
            if (loadingIt->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                auto model = loadingIt->second.get();
                loading_.erase(loadingIt);
                if (model) {
                    addToCache(name, model);
                    return model;
                }
            }
            return nullptr; // Still loading
        }

        // Start loading
        startAsyncLoad(name);
        return nullptr;
    }

    bool ModelManager::isModelReady(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.find(name) != cache_.end();
    }

    bool ModelManager::isModelLoading(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        LOGI("Checking if model is loading: %s", name.c_str());
        return loading_.find(name) != loading_.end();
    }

    void ModelManager::preloadEssentials() {
        // Load first 3 models synchronously
        std::vector<std::string> essentials = {"Akita Inu", "Shiba Inu", "Pitbull"};
        for (const auto& name : essentials) {
            if (cache_.size() >= MAX_LOADED_MODELS) break;
            LOGI("Preloading model: %s", name.c_str());
            auto model = loadModel(name);
            if (model) {
                std::lock_guard<std::mutex> lock(mutex_);
                addToCache(name, model);
            }
        }
    }

    void ModelManager::preloadAsync(const std::vector<std::string>& models) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& name : models) {
            if (cache_.find(name) == cache_.end() && loading_.find(name) == loading_.end()) {
                startAsyncLoad(name);
            }
        }
    }

    const std::vector<std::string>& ModelManager::getAvailableModels() const {
        return AVAILABLE_MODELS;
    }

    void ModelManager::clearAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        accessOrder_.clear();
        loading_.clear();
    }

    void ModelManager::addToCache(const std::string& name, std::shared_ptr<VeModel> model) {
        // Evict oldest if needed
        while (cache_.size() >= MAX_LOADED_MODELS && !accessOrder_.empty()) {
            evictOldest();
        }

        cache_[name] = model;
        accessOrder_.push_front(name);
    }

    void ModelManager::evictOldest() {
        if (!accessOrder_.empty()) {
            std::string oldest = accessOrder_.back();
            accessOrder_.pop_back();
            cache_.erase(oldest);
        }
    }

    std::shared_ptr<VeModel> ModelManager::loadModel(const std::string& name) {
        std::string path = getModelPath(name);
        if (path.empty()) return nullptr;

        try {
            return VeModel::createModelFromFile(device_, assetManager_, *modelDescriptorPool, path);
        } catch (...) {
            LOGE("Error: creating model %s", name.c_str());
            return nullptr;
        }
    }

    void ModelManager::startAsyncLoad(const std::string& name) {
        loading_[name] = std::async(std::launch::async, [this, name]() {
            return loadModel(name);
        });
    }

    std::string ModelManager::getModelPath(const std::string& name) const {
        auto it = MODEL_PATHS.find(name);
        return (it != MODEL_PATHS.end()) ? it->second : "";
    }

    void ModelManager::initializeModels(VeDevice& device, AAssetManager* assetManager) {
        preloadEssentials();
        preloadAsync({"Beagle", "Border Collie"});
    }


}