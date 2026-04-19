#pragma once

#include "types.hpp"
#include "board_detector.hpp"
#include "board_tracker.hpp"
#include "board_analyzer.hpp"
#include "system_configuration.hpp"
#include "config_manager.hpp"

class InspectionSystem
{
public:
    InspectionSystem();

    void processFrame(const cv::Mat& frame);

    void draw(cv::Mat& frame) const;

    const SystemStats& getStats() const { return stats_; }
    StatsSnapshot getStatsSnapshot() const;

    // Мониторинг остановки линии
    bool isLineStopped() const;
    int getSecondsSinceLastMotion() const;

    InspectionSystemConfig cfg;
    AppConfig loadConfig();
    void applyConfig(const AppConfig& config);
    AppConfig getConfig() const;

private:
    BoardDetector detector_;
    BoardTracker tracker_;
    BoardAnalyzer analyzer_;

    SystemStats stats_;

    int frameCount_ = 0;

    // TODO: Добавить логирование и сохранение проанализированных досок
    // std::unique_ptr<Logger> logger_;
};
