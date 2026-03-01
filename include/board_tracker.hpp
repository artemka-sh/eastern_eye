#pragma once

#include "types.hpp"
#include <vector>
#include <memory>
#include <opencv2/video/tracking.hpp>
#include <opencv2/tracking.hpp>

class BoardTracker {
public:
    BoardTracker();

    // Принимает кадр и свежие детекции (вектор может быть пустым, если детектор в этом кадре отдыхал)
    void update(const cv::Mat& frame, const std::vector<DetectedBoard>& detections);

    std::vector<BoardTrack>& getActiveTracks() { return activeTracks_; }
    const std::vector<BoardTrack>& getActiveTracks() const { return activeTracks_; }

    int getTotalCounted() const { return totalCounted_; }

    void setCountLineX(int x) { countLineX_ = x; }
    void setMinFramesStable(int frames) { minFramesStable_ = frames; }

private:
    // Теперь методы работают с DetectedBoard, а не с Rect!
    void matchDetectionsToTracks(const cv::Mat& frame,
                                 const std::vector<DetectedBoard>& detections,
                                 std::vector<bool>& matched);
    void createNewTrack(const cv::Mat& frame, const DetectedBoard& board);
    void countBoards();
    void cleanupLostTracks(int frameWidth);

    float computeIoU(const cv::Rect& a, const cv::Rect& b) const;

    std::vector<BoardTrack> activeTracks_;

    int nextId_ = 0;
    int totalCounted_ = 0;

    float minIouMatch_ = 0.3f;
    int maxFramesLost_ = 15;
    int countLineX_ = 800;
    int minFramesStable_ = 5;
};