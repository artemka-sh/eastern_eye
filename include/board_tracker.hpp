#pragma once

#include "types.hpp"
#include <memory>

class BoardTracker {
public:
    BoardTracker();
    
    void update(const cv::Mat& frame, const std::vector<cv::Rect>& detections);
    
    std::vector<BoardTrack>& getActiveTracks() { return activeTracks_; }
    const std::vector<BoardTrack>& getActiveTracks() const { return activeTracks_; }
    
    int getTotalCounted() const { return totalCounted_; }
    
    void setCountLineX(int x) { countLineX_ = x; }
    void setMinFramesStable(int frames) { minFramesStable_ = frames; }

private:
    void matchDetectionsToTracks(const cv::Mat& frame,  // ← Добавлен параметр frame
                                 const std::vector<cv::Rect>& detections,
                                 std::vector<bool>& matched);
    void createNewTrack(const cv::Mat& frame, const cv::Rect& bbox);
    void countBoards();
    void cleanupLostTracks(int frameWidth);

    float computeIoU(const cv::Rect& a, const cv::Rect& b) const;
    cv::Point2f getCentroid(const cv::Rect& bbox) const;

    std::vector<BoardTrack> activeTracks_;

    int nextId_ = 0;
    int totalCounted_ = 0;

    float minIouMatch_ = 0.3f;
    int maxFramesLost_ = 15;
    int countLineX_ = 800;
    int minFramesStable_ = 5;
};