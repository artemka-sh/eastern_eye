#pragma once

#include "types.hpp"
#include "system_configuration.hpp"
#include <opencv2/imgproc.hpp>

class BoardAnalyzer
{
public:
    BoardAnalyzer();

    void analyze(const cv::Mat& frame, BoardTrack& track);

    void setRules(const BoardAnalyzerConfig& rules) { cfg = rules; }

    BoardAnalyzerConfig cfg;

private:
    cv::Scalar analyzeColor(const cv::Mat& boardROI);
    std::vector<Defect> detectDefectsSimple(const cv::Mat& boardROI);
    std::string classifyBoard(const cv::Scalar& avgColor, float defectRatio);
};
