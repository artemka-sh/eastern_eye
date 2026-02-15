#include "board_detector.hpp"

BoardDetector::BoardDetector() {
    // MOG2 хорошо работает для статичного фона
    bgSub_ = cv::createBackgroundSubtractorMOG2(500, 16, false);
    
    morphKernel_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
}

std::vector<cv::Rect> BoardDetector::detect(const cv::Mat& frame) {
    cv::Mat fgMask;
    bgSub_->apply(frame, fgMask);
    
    // Морфология для очистки шума
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_CLOSE, morphKernel_);
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_OPEN, morphKernel_);
    
    // Поиск контуров
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    std::vector<cv::Rect> boards;
    
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        
        // Фильтр по площади
        if (area < minArea_ || area > maxArea_)
            continue;
        
        cv::Rect bbox = cv::boundingRect(contour);
        
        // Фильтр по форме (доски вытянутые)
        float aspectRatio = static_cast<float>(bbox.width) / bbox.height;
        if (aspectRatio < minAspectRatio_ || aspectRatio > maxAspectRatio_)
            continue;
        
        // Фильтр по позиции (доски на конвейере, а не мусор сбоку)
        if (bbox.y < minY_ || bbox.y > maxY_)
            continue;
        
        boards.push_back(bbox);
    }
    
    return boards;
}