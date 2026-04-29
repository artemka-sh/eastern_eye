#pragma once

#include "types.hpp"
#include "system_configuration.hpp"
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/imgproc.hpp>

class BoardAnalyzer
{
public:
    BoardAnalyzer();

    void analyze(const cv::Mat& frame, BoardTrack& track);

    void setRules(const BoardAnalyzerConfig& rules) { cfg = rules; }
    cv::Mat getROImatrix(const cv::RotatedRect& rBox, const cv::Mat& frame) const noexcept;

    BoardAnalyzerConfig cfg;

private:
    cv::Scalar analyzeColor(const cv::Mat& boardROI);
    std::vector<Defect> detectDefects(const cv::Mat& boardROI);
    std::string classifyBoard(const cv::Scalar& avgColor, float defectRatio);
    cv::Mat preprocess(const cv::Mat& boardROI, LetterboxInfo& info) const;
    std::vector<Defect> postprocess(const cv::Mat& rawOutput, cv::Size roiSize,
                                    const LetterboxInfo& info) const;

    void drawDefects(cv::Mat& image, const std::vector<Defect>& defects) const;
    void drawLabel(cv::Mat& image, const std::string& label, int left, int top) const;

    cv::dnn::Net net;
    int FONT_FACE = cv::FONT_HERSHEY_SIMPLEX;
};
