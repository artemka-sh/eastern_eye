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

    // Настройки зоны анализа
    void setAnalysisZone(int xMin = 0, int xMax = 0, int yMin = 0, int yMax = 0)
    {
        cfg.analysisZoneXMin_ = xMin;  cfg.analysisZoneXMax_ = xMax;
        cfg.analysisZoneYMin_ = yMin;  cfg.analysisZoneYMax_ = yMax;
    }

    // Мониторинг остановки линии
    void setLineStopThreshold(int seconds) { cfg.lineStopThresholdSec_ = seconds; }
    bool isLineStopped() const;
    int getSecondsSinceLastMotion() const;

    InspectionSystemConfig cfg;
    void syncConfigs();

private:
    bool isInAnalysisZone(const BoardTrack& track) const;

    BoardDetector detector_;
    BoardTracker tracker_;
    BoardAnalyzer analyzer_;

    SystemStats stats_;

    int frameCount_ = 0;

    // TODO: Добавить логирование и сохранение проанализированных досок
    // std::unique_ptr<Logger> logger_;
};
