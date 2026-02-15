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

// Состояние отслеживаемой доски
struct BoardTrack {
    int id;
    cv::Rect bbox;
    cv::Point2f centroid;
    
    cv::Ptr<cv::Tracker> tracker;  /////////////////////////////////////////////////
    
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