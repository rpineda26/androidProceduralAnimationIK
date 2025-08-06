#include "dog_classifier.hpp"
#include <opencv2/imgproc.hpp>
#include <android/log.h>

#define LOG_TAG "DogClassifier"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

DogClassifier::DogClassifier() {
    // Initialize HOG descriptor with EXACT same parameters as training
    hog.winSize = Size(224, 224);  // TARGET_WIDTH x TARGET_HEIGHT
    hog.blockSize = Size(32, 32);  // HOG_CELL_SIZE * HOG_BLOCK_SIZE = 16 * 2
    hog.blockStride = Size(16, 16); // HOG_CELL_SIZE
    hog.cellSize = Size(16, 16);   // HOG_CELL_SIZE
    hog.nbins = 9;                 // HOG_BINS

    model_loaded = false;
    LOGI("DogClassifier initialized with HOG parameters matching training");
}

bool DogClassifier::loadModel(AAssetManager* assetManager, const string& rf_filename, const string& breeds_filename) {
    try {
        // Load Random Forest model from assets
        AAsset* rf_asset = AAssetManager_open(assetManager, rf_filename.c_str(), AASSET_MODE_BUFFER);
        if (!rf_asset) {
            LOGE("Failed to open RF asset: %s", rf_filename.c_str());
            return false;
        }

        size_t rf_length = AAsset_getLength(rf_asset);
        const void* rf_buffer = AAsset_getBuffer(rf_asset);

        // Write to temporary file for OpenCV to read
        string temp_rf = "/data/data/temp_rf.xml";
        FILE* rf_file = fopen(temp_rf.c_str(), "wb");
        if (rf_file) {
            fwrite(rf_buffer, 1, rf_length, rf_file);
            fclose(rf_file);
        }
        AAsset_close(rf_asset);

        random_forest = RTrees::load(temp_rf);
        remove(temp_rf.c_str()); // Clean up temp file

        if (random_forest.empty()) {
            LOGE("Failed to load Random Forest model");
            return false;
        }

        // Load breed information from assets
        AAsset* breeds_asset = AAssetManager_open(assetManager, breeds_filename.c_str(), AASSET_MODE_BUFFER);
        if (!breeds_asset) {
            LOGE("Failed to open breeds asset: %s", breeds_filename.c_str());
            return false;
        }

        size_t breeds_length = AAsset_getLength(breeds_asset);
        const void* breeds_buffer = AAsset_getBuffer(breeds_asset);

        // Write to temporary file for OpenCV to read
        string temp_breeds = "/data/data/temp_breeds.xml";
        FILE* breeds_file = fopen(temp_breeds.c_str(), "wb");
        if (breeds_file) {
            fwrite(breeds_buffer, 1, breeds_length, breeds_file);
            fclose(breeds_file);
        }
        AAsset_close(breeds_asset);

        FileStorage fs(temp_breeds, FileStorage::READ);
        if (!fs.isOpened()) {
            LOGE("Failed to open breeds file");
            remove(temp_breeds.c_str());
            return false;
        }

        int breed_count;
        fs["breed_count"] >> breed_count;
        fs["breed_names"] >> breed_names;
        fs["breed_labels"] >> breed_labels;
        fs["breed_ids"] >> breed_ids;
        fs.release();

        remove(temp_breeds.c_str()); // Clean up temp file

        model_loaded = true;
        LOGI("Model loaded successfully with %d breeds", breed_count);
        return true;

    } catch (const Exception& e) {
        LOGE("OpenCV exception while loading model: %s", e.what());
        return false;
    }
    return true;
}

// Preprocess image EXACTLY like training
Mat DogClassifier::preprocessImage(const Mat& image) {
    Mat processed;

    // Convert to RGB if needed (matching training)
    if (image.channels() == 4) {
        cvtColor(image, processed, COLOR_RGBA2RGB);
    } else if (image.channels() == 3) {
        // Assume input might be BGR, convert to RGB to match training
        cvtColor(image, processed, COLOR_BGR2RGB);
    } else {
        processed = image.clone();
    }

    // Smart resize maintaining aspect ratio (EXACTLY like training)
    const int TARGET_WIDTH = 224;
    const int TARGET_HEIGHT = 224;

    double scale = min(double(TARGET_WIDTH) / processed.cols,
                       double(TARGET_HEIGHT) / processed.rows);

    int new_width = int(processed.cols * scale);
    int new_height = int(processed.rows * scale);

    resize(processed, processed, Size(new_width, new_height), 0, 0, INTER_LANCZOS4);

    // Center on canvas (EXACTLY like training)
    Mat canvas = Mat::zeros(TARGET_HEIGHT, TARGET_WIDTH, processed.type());
    int x_offset = (TARGET_WIDTH - new_width) / 2;
    int y_offset = (TARGET_HEIGHT - new_height) / 2;

    processed.copyTo(canvas(Rect(x_offset, y_offset, new_width, new_height)));

    // Light noise reduction (EXACTLY like training)
    GaussianBlur(canvas, canvas, Size(3, 3), 0.5);

    return canvas;
}

// Extract color histogram EXACTLY like training
vector<float> DogClassifier::extractColorHistogram(const Mat& image) {
    vector<float> features;

    if (image.channels() == 3) {
        vector<Mat> channels;
        split(image, channels);

        // Same histogram size as training (16 bins, not 32!)
        for (int c = 0; c < 3; c++) {
            Mat hist;
            int histSize = 16;  // MUST match training
            float range[] = {0, 256};
            const float* histRange = {range};

            calcHist(&channels[c], 1, 0, Mat(), hist, 1, &histSize, &histRange);
            normalize(hist, hist, 0, 1, NORM_L2);  // L2 norm like training

            for (int i = 0; i < histSize; i++) {
                features.push_back(hist.at<float>(i));
            }
        }
    }

    return features;
}

// Extract texture features EXACTLY like training
vector<float> DogClassifier::extractTextureFeatures(const Mat& gray) {
    vector<float> features;

    // Simple but effective texture measures (EXACTLY like training)
    Mat laplacian;
    Laplacian(gray, laplacian, CV_32F);

    Scalar mean, stddev;
    meanStdDev(laplacian, mean, stddev);

    features.push_back(stddev[0]);  // Texture variance

    // Edge density (EXACTLY like training)
    Mat edges;
    Canny(gray, edges, 50, 150);
    int edge_pixels = countNonZero(edges);
    float edge_density = float(edge_pixels) / (gray.rows * gray.cols);
    features.push_back(edge_density);

    return features;
}

// Main feature extraction - EXACTLY like training
vector<float> DogClassifier::extractFeatures(const Mat& image) {
    if (image.empty()) {
        LOGE("Empty image provided to extractFeatures");
        return vector<float>();
    }

    Mat processed = preprocessImage(image);
    Mat gray;

    // Convert to grayscale for HOG (EXACTLY like training)
    if (processed.channels() == 3) {
        cvtColor(processed, gray, COLOR_RGB2GRAY);
    } else {
        gray = processed.clone();
    }

    vector<float> all_features;

    // 1. HOG features (main feature) - EXACTLY like training
    vector<float> hog_descriptors;
    try {
        hog.compute(gray, hog_descriptors);
        all_features.insert(all_features.end(), hog_descriptors.begin(), hog_descriptors.end());
        LOGI("HOG features extracted: %zu", hog_descriptors.size());
    } catch (const Exception& e) {
        LOGE("HOG extraction failed: %s", e.what());
        // Fill with zeros if HOG fails (matching training behavior)
        all_features.resize(hog.getDescriptorSize(), 0.0f);
    }

    // 2. Simple color histogram (lightweight) - EXACTLY like training
    vector<float> color_features = extractColorHistogram(processed);
    all_features.insert(all_features.end(), color_features.begin(), color_features.end());
    LOGI("Color features extracted: %zu", color_features.size());

    // 3. Basic texture features - EXACTLY like training
    vector<float> texture_features = extractTextureFeatures(gray);
    all_features.insert(all_features.end(), texture_features.begin(), texture_features.end());
    LOGI("Texture features extracted: %zu", texture_features.size());

    LOGI("Total features extracted: %zu", all_features.size());
    return all_features;
}

pair<string, float> DogClassifier::classifyImage(const Mat& image) {
    if (!model_loaded) {
        LOGE("Model not loaded");
        return make_pair("Model not loaded", 0.0f);
    }

    if (image.empty()) {
        LOGE("Empty image provided");
        return make_pair("Empty image", 0.0f);
    }

    try {
        vector<float> features = extractFeatures(image);
        if (features.empty()) {
            LOGE("Feature extraction failed - empty feature vector");
            return make_pair("Feature extraction failed", 0.0f);
        }

        LOGI("Features extracted successfully: %zu dimensions", features.size());

        // Convert to OpenCV Mat format
        Mat feature_mat(1, features.size(), CV_32FC1);
        for (size_t i = 0; i < features.size(); i++) {
            feature_mat.at<float>(0, i) = features[i];
        }

        // Predict with Random Forest
        Mat votes;
        float prediction = random_forest->predict(feature_mat, votes);

        // Get confidence (vote percentage)
        float confidence = 0.0f;
        if (!votes.empty()) {
            double max_votes;
            minMaxLoc(votes, nullptr, &max_votes);
            // Confidence as percentage of trees that voted for this class
            confidence = float(max_votes);
        }

        // Convert prediction to breed name
        int predicted_label = (int)prediction;
        string breed_name = "Unknown";

        for (size_t i = 0; i < breed_labels.size(); i++) {
            if (breed_labels[i] == predicted_label) {
                breed_name = breed_names[i];
                break;
            }
        }

        LOGI("Prediction: %s (label: %d, confidence: %.3f)",
             breed_name.c_str(), predicted_label, confidence);

        return make_pair(breed_name, confidence);

    } catch (const Exception& e) {
        LOGE("Classification error: %s", e.what());
        return make_pair("Classification failed", 0.0f);
    }
}

bool DogClassifier::isModelLoaded() const {
    return model_loaded;
}