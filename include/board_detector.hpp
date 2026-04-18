#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include "types.hpp"
#include "system_configuration.hpp"

class BoardDetector
{
public:
    BoardDetector();

    // Главный метод: теперь возвращает вектор сложных структур
    std::vector<DetectedBoard> detect(const cv::Mat& frame);

    // Настройки детектора
    void setMinArea(double area) { cfg.minRelativeArea_ = area; }
    void setMaxArea(double area) { cfg.maxRelativeArea_ = area; }
    void setMinAspectRatio(float ratio) { cfg.minAspectRatio_ = ratio; }
    void setMaxAspectRatio(float ratio) { cfg.maxAspectRatio_ = ratio; }
    void setLabAThreshold(int thresh) { cfg.labAThreshold_ = thresh; }

    BoardDetectorConfig cfg;

private:
    // Внутренний метод для расчета угла между тремя точками
    double calculateAngle(cv::Point2f A, cv::Point2f B, cv::Point2f C);

    cv::Mat morphKernel_;
};
