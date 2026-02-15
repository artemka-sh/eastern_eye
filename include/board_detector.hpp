#pragma once

#include "types.hpp"
#include <opencv2/video/background_segm.hpp>
#include <opencv2/imgproc.hpp>

class BoardDetector {
public:
    BoardDetector();
    
    std::vector<cv::Rect> detect(const cv::Mat& frame);
    
    // Настройки детектора (можно менять на лету)
    void setMinArea(double area) { minArea_ = area; }
    void setMaxArea(double area) { maxArea_ = area; }
    void setMinAspectRatio(float ratio) { minAspectRatio_ = ratio; }
    void setMaxAspectRatio(float ratio) { maxAspectRatio_ = ratio; }
    
private:
    cv::Ptr<cv::BackgroundSubtractor> bgSub_;
    
    // Параметры фильтрации (настраиваются под конвейер)
    double minArea_ = 5000.0;
    double maxArea_ = 150000.0;
    float minAspectRatio_ = 2.0f;
    float maxAspectRatio_ = 15.0f;
    int minY_ = 50;   // ROI по высоте
    int maxY_ = 1000;
    
    cv::Mat morphKernel_;
};