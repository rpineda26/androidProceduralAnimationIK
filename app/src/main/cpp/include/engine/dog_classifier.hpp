//
// Created by Ralph Dawson Pineda on 6/5/25.
//

#ifndef VULKANANDROID_DOG_CLASSIFIER_HPP
#define VULKANANDROID_DOG_CLASSIFIER_HPP
#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <android/asset_manager.h>
#include <string>
#include <vector>
using namespace cv;
using namespace cv::ml;
using namespace std;

class DogClassifier {
private:
    Ptr<RTrees> random_forest;
    HOGDescriptor hog;  // Add HOG descriptor
    vector<string> breed_names;
    vector<int> breed_labels;
    vector<string> breed_ids;
    bool model_loaded;

    // Image preprocessing - matching training exactly
    Mat preprocessImage(const Mat& image);

    // Feature extraction methods - matching training exactly
    vector<float> extractColorHistogram(const Mat& image);
    vector<float> extractTextureFeatures(const Mat& gray);

public:
    DogClassifier();  // Initialize HOG parameters

    bool loadModel(AAssetManager* assetManager,
                   const string& rf_filename,
                   const string& breeds_filename);

    vector<float> extractFeatures(const Mat& image);  // Changed return type
    pair<string, float> classifyImage(const Mat& image);
    bool isModelLoaded() const;
};

#endif //VULKANANDROID_DOG_CLASSIFIER_HPP
