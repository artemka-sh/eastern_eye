#pragma once

#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp> // Для cv::Tracker
#include <opencv2/tracking.hpp>
#include <string>
#include <vector>
#include <chrono>
#include <map>

// Информация о дефекте на доске
struct Defect {
    cv::Rect bbox;
    float severity;  // 0.0-1.0
    std::string type;
};

// Геометрия (Физическое состояние в конкретный момент)
struct DetectedBoard {
    cv::RotatedRect rBox;             // Повернутый прямоугольник (центр, габариты, общий наклон)
    std::vector<cv::Point2f> corners; // 4 точных угла доски (субпиксельные)
    std::vector<double> angles;       // 4 угла между гранями (в градусах)
    bool isGeometryValid;             // Удалось ли найти ровно 4 угла (не кусок мусора)
};


// Состояние отслеживаемой доски
struct BoardTrack {
    int id;
    DetectedBoard geometry;
    
    cv::Ptr<cv::Tracker> tracker;
    
    int framesSeen = 0;
    int framesLost = 0;
    bool counted = false;
    bool analyzed = false;
    
    // Результаты анализа
    cv::Scalar avgColor;
    std::vector<Defect> defects;
    std::string category;
    float defectRatio = 0.0f;
    
    // TODO: Добавить "отпечаток пальца" доски для реидентификации
    // BoardFingerprint fingerprint;
    cv::Point2f getCentroid() const { return geometry.rBox.center; }
    cv::Rect getBoundingBox() const { return geometry.rBox.boundingRect(); }
};

// Статистика работы системы
struct SystemStats {
    int totalDetected = 0;
    int totalCounted = 0;
    std::map<std::string, int> categoryCounts;
    std::chrono::steady_clock::time_point lastMotionTime;
    
    void reset() {
        totalDetected = 0;
        totalCounted = 0;
        categoryCounts.clear();
        lastMotionTime = std::chrono::steady_clock::now();
    }
};
