#include "inspection_system.hpp"
#include <iostream>

InspectionSystem::InspectionSystem() {
    stats_.lastMotionTime = std::chrono::steady_clock::now();
}

void InspectionSystem::processFrame(const cv::Mat& frame) {
    frameCount_++;
    
    // Периодическая детекция досок
    std::vector<DetectedBoard> detections;
    if (frameCount_ % detectInterval_ == 0) {
        detections = detector_.detect(frame);
        //TODO stats_.totalDetected = static_cast<int>(detections.size());
    }
    
    // Обновление трекера
    tracker_.update(frame, detections);
    
    // Проверка движения
    if (!tracker_.getActiveTracks().empty()) {
        stats_.lastMotionTime = std::chrono::steady_clock::now();
    }

    // Анализ досок в зоне анализа
    for (auto& track : tracker_.getActiveTracks()) {
        if (isInAnalysisZone(track) && !track.analyzed) {
            analyzer_.analyze(frame, track);

            // Обновляем статистику категорий только для посчитанных
            if (track.counted) {
                stats_.categoryCounts[track.category]++;
            }
        }
    }

    // Обновляем общий счётчик
    stats_.totalCounted = tracker_.getTotalCounted();
}

void InspectionSystem::draw(cv::Mat& frame) const {
    // Рисуем линию подсчёта
    cv::line(frame, cv::Point(800, 0), cv::Point(800, frame.rows), 
             cv::Scalar(0, 255, 255), 2);
    
    // Рисуем зону анализа
    cv::rectangle(frame, 
                 cv::Point(analysisZoneXMin_, 0),
                 cv::Point(analysisZoneXMax_, frame.rows),
                 cv::Scalar(255, 255, 0), 2);
    
    // Рисуем треки
    for (const auto& track : tracker_.getActiveTracks()) {
        cv::Scalar color;
        
        if (track.counted) {
            color = cv::Scalar(0, 255, 0);  // Зелёный - посчитана
        } else if (track.framesLost > 0) {
            color = cv::Scalar(0, 165, 255);  // Оранжевый - потеря
        } else {
            color = cv::Scalar(255, 0, 0);  // Синий - активна
        }
        
        cv::rectangle(frame, track.getBoundingBox(), color, 2);
        
        // ID и категория
        std::string label = "ID:" + std::to_string(track.id);
        if (track.analyzed) {
            label += " " + track.category;
        }
        
        cv::putText(frame, label, 
                   cv::Point(track.getBoundingBox().x, track.getBoundingBox().y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
        
        // Центроид
        cv::circle(frame, track.getCentroid(), 4, color, -1);
        
        // Дефекты (если есть)
        if (track.analyzed && !track.defects.empty()) {
            for (const auto& defect : track.defects) {
                cv::Rect globalDefect = defect.bbox;
                globalDefect.x += track.getBoundingBox().x;
                globalDefect.y += track.getBoundingBox().y;
                cv::rectangle(frame, globalDefect, cv::Scalar(0, 0, 255), 1);
            }
        }
    }
    
    // Статистика
    std::string stats = "Active: " + std::to_string(tracker_.getActiveTracks().size()) +
                       " | Counted: " + std::to_string(stats_.totalCounted);
    cv::putText(frame, stats, cv::Point(10, 30),
               cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
    
    // Категории
    int y = 60;
    for (const auto& [category, count] : stats_.categoryCounts) {
        std::string catStats = category + ": " + std::to_string(count);
        cv::putText(frame, catStats, cv::Point(10, y),
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        y += 30;
    }
    
    // Предупреждение об остановке
    if (isLineStopped()) {
        std::string warning = "LINE STOPPED! " + std::to_string(getSecondsSinceLastMotion()) + "s";
        cv::putText(frame, warning, cv::Point(10, frame.rows - 20),
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 3);
    }
}

bool InspectionSystem::isInAnalysisZone(const BoardTrack& track) const {
    return track.getCentroid().x >= analysisZoneXMin_ &&
           track.getCentroid().x <= analysisZoneXMax_;
}

bool InspectionSystem::isLineStopped() const {
    return getSecondsSinceLastMotion() > lineStopThresholdSec_;
}

int InspectionSystem::getSecondsSinceLastMotion() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - stats_.lastMotionTime);
    return static_cast<int>(elapsed.count());
}