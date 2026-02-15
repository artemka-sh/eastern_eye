#pragma once

#include "types.hpp"
#include <opencv2/imgproc.hpp>

class BoardAnalyzer {
public:
    BoardAnalyzer();
    
    void analyze(const cv::Mat& frame, BoardTrack& track);
    
    // Настройки классификации (задаются по стандартам предприятия)
    struct ClassificationRules {
        float gradeA_minLightness = 70.0f;
        float gradeA_maxDefectRatio = 0.05f;
        
        float gradeB_minLightness = 60.0f;
        float gradeB_maxDefectRatio = 0.15f;
    };
    
    void setRules(const ClassificationRules& rules) { rules_ = rules; }
    
private:
    cv::Scalar analyzeColor(const cv::Mat& boardROI);
    std::vector<Defect> detectDefectsSimple(const cv::Mat& boardROI);
    std::string classifyBoard(const cv::Scalar& avgColor, float defectRatio);
    
    ClassificationRules rules_;
    
    // TODO: Заменить на DL модель для точной детекции дефектов
    // cv::dnn::Net defectDetector_;
};