#include "inspection_system.hpp"
#include <algorithm>

InspectionSystem::InspectionSystem() {
    stats_.lastMotionTime = std::chrono::steady_clock::now();
}

void InspectionSystem::processFrame(const cv::Mat& frame) {
    frameCount_++;
    
    std::vector<DetectedBoard> detections;
    bool qualityDetectMoment = frameCount_ % cfg.detectInterval_ == 0;
    if (qualityDetectMoment) {
        detections = detector_.detect(frame);
        tracker_.update(frame, detections);
    } else {

        tracker_.update(frame, {}); // пустой вектор — KCF просто двигает треки
    }
    
    // Проверка движения
    if (!tracker_.getActiveTracks().empty()) {
        stats_.lastMotionTime = std::chrono::steady_clock::now();
    }

    // Анализ досок (детектор уже отфильтровал по позиции на конвейере)
    for (auto& track : tracker_.getActiveTracks())
    {
        if (!track.analyzed)
        if (qualityDetectMoment && track.geometry.rBox.center.y < frame.rows / 2)
        { // посередине+-
            analyzer_.analyze(frame, track);

            if (track.counted)
                stats_.categoryCounts[track.category]++;
        }
    }

    // Обновляем общий счётчик
    stats_.totalCounted = tracker_.getTotalCounted();
    stats_.totalDetected = static_cast<int>(detections.size());

}

void InspectionSystem::draw(cv::Mat& frame) const {

    // Рисуем позицию конвейера (параметры живут в детекторе)
    const auto& dcfg = detector_.cfg;
    int x1 = std::clamp(dcfg.minX_, 0, frame.cols);
    int y1 = std::clamp(dcfg.minY_, 0, frame.rows);
    int x2 = std::clamp(dcfg.maxX_, 0, frame.cols);
    int y2 = std::clamp(dcfg.maxY_, 0, frame.rows);
    cv::rectangle(frame, {x1, y1}, {x2, y2}, cv::Scalar(Color::CYAN), 2);
    
    // Рисуем треки
    for (const auto& track : tracker_.getActiveTracks()) {
        cv::Scalar color;
        
        if (track.counted) {
            color = cv::Scalar(Color::GREEN);  // Зелёный - посчитана
        } else if (track.framesLost > 0) {
            color = cv::Scalar(Color::ORANGE);  // Оранжевый - потеря
        } else {
            color = cv::Scalar(Color::BLUE);  // Синий - активна
        }
        
        cv::rectangle(frame, track.getBoundingBox(), color, 2);
        
        // ID и категория
        std::string label = "ID:" + std::to_string(track.id);
        if (track.analyzed) {
            label += " " + track.category;
        }
        label += " r_area:" + std::to_string(track.geometry.relativeArea_);
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
                cv::rectangle(frame, globalDefect, cv::Scalar(Color::RED), 1);
            }
        }
    }
    
    // Статистика
    std::string stats = "Active: " + std::to_string(tracker_.getActiveTracks().size()) +
                       " | Counted: " + std::to_string(stats_.totalCounted);
    cv::putText(frame, stats, cv::Point(10, 30),
               cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(Color::GREEN), 2);
    
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

StatsSnapshot InspectionSystem::getStatsSnapshot() const
{
    return {
        .totalCounted       = stats_.totalCounted,
        .totalDetected      = stats_.totalDetected,
        .activeTracks       = static_cast<int>(tracker_.getActiveTracks().size()),
        .secondsSinceMotion = getSecondsSinceLastMotion(),
        .lineStopped        = isLineStopped(),
        .categoryCounts     = stats_.categoryCounts,
    };
}

bool InspectionSystem::isLineStopped() const {
    return getSecondsSinceLastMotion() > cfg.lineStopThresholdSec_;
}

int InspectionSystem::getSecondsSinceLastMotion() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - stats_.lastMotionTime);
    return static_cast<int>(elapsed.count());
}

AppConfig InspectionSystem::loadConfig()
{
    AppConfig config{cfg, detector_.cfg, tracker_.cfg, analyzer_.cfg};
    ConfigManager::load(config);
    applyConfig(config);
    ConfigManager::save(config);
    return config;
}

AppConfig InspectionSystem::getConfig() const
{
    return AppConfig{cfg, detector_.cfg, tracker_.cfg, analyzer_.cfg};
}

void InspectionSystem::applyConfig(const AppConfig& config)
{
    cfg               = config.inspectionSystemConfig;
    detector_.cfg     = config.boardDetectorConfig;
    tracker_.cfg      = config.boardTrackerConfig;
    analyzer_.cfg     = config.boardAnalyzerConfig;
}