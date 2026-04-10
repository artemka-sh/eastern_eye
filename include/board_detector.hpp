#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <board_tracker.hpp>

class BoardDetector {
public:
    BoardDetector();

    // Главный метод: теперь возвращает вектор сложных структур
    std::vector<DetectedBoard> detect(const cv::Mat& frame);

    // Настройки детектора
    void setMinArea(double area) { minRelativeArea_ = area; }
    void setMaxArea(double area) { maxRelativeArea_ = area; }
    void setMinAspectRatio(float ratio) { minAspectRatio_ = ratio; }
    void setMaxAspectRatio(float ratio) { maxAspectRatio_ = ratio; }
    void setLabAThreshold(int thresh) { labAThreshold_ = thresh; }

private:
    // Внутренний метод для расчета угла между тремя точками
    double calculateAngle(cv::Point2f A, cv::Point2f B, cv::Point2f C);

    // Параметры фильтрации
    double minRelativeArea_ = 0.05; //5%
    double maxRelativeArea_ = DBL_MAX;
    float minAspectRatio_ = 0;
    float maxAspectRatio_ = FLT_MAX;
    int minY_ = 0;
    int maxY_ = INT_MAX;

    // Порог для канала 'a' в пространстве LAB (отсечение зеленого конвейера)
    // Значение 128 - нейтраль. Зеленый < 128, дерево > 128.
    int labAThreshold_ = 130;

    cv::Mat morphKernel_;
};