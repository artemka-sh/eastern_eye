#pragma once

#include "types.hpp"
#include "board_detector.hpp"
#include "board_tracker.hpp"
#include "board_analyzer.hpp"

class InspectionSystem {
public:
    InspectionSystem();
    
    void processFrame(const cv::Mat& frame);
    
    void draw(cv::Mat& frame) const;
    
    const SystemStats& getStats() const { return stats_; }
    
    // Настройки зоны анализа
    void setAnalysisZone(int xMin = 0, int xMax = 0, int yMin = 0, int yMax = 0) {
        analysisZoneXMin_ = xMin;      analysisZoneXMax_ = xMax;
        analysisZoneYMin_ = yMin;      analysisZoneYMax_ = yMax;
    }
    
    // Мониторинг остановки линии
    void setLineStopThreshold(int seconds) { lineStopThresholdSec_ = seconds; }
    bool isLineStopped() const;
    int getSecondsSinceLastMotion() const;
    
private:
    bool isInAnalysisZone(const BoardTrack& track) const;
    
    BoardDetector detector_;
    BoardTracker tracker_;
    BoardAnalyzer analyzer_;
    
    SystemStats stats_;
    
    int frameCount_ = 0;
    int detectInterval_ = 10;
    
    int analysisZoneXMin_; int analysisZoneXMax_;
    int analysisZoneYMin_; int analysisZoneYMax_;
    
    int lineStopThresholdSec_ = 60;  // Порог остановки (секунды)
    
    // TODO: Добавить логирование и сохранение проанализированных досок
    // std::unique_ptr<Logger> logger_;
};