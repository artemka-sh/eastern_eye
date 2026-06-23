#pragma once

#include "types.hpp"
#include <vector>
#include <unordered_set>
#include <memory>
#include <opencv2/video/tracking.hpp>

#include "system_configuration.hpp"

class BoardTracker
{
public:
    BoardTracker();

    void update(const std::vector<DetectedBoard>& detections);

    std::vector<BoardTrack>& getActiveTracks() { return activeTracks_; }
    const std::vector<BoardTrack>& getActiveTracks() const { return activeTracks_; }

    int getTotalCounted() const { return totalCounted_; }

    void setCountLineX(int x) { cfg.countLineX_ = x; }
    void setMinFramesStable(int frames) { cfg.minFramesStable_ = frames; }

    BoardTrackerConfig cfg;

private:
    void matchDetectionsToTracks(const cv::Mat& frame,
                                 const std::vector<DetectedBoard>& detections,
                                 std::vector<bool>& matched,
                                 std::unordered_set<int>& toRemove);
    void createNewTrack(const DetectedBoard& board);
    void countBoards();
    void cleanupLostTracks();

    float computeIoU(const cv::RotatedRect& a, const cv::RotatedRect& b) const;

    std::vector<BoardTrack> activeTracks_;

    int nextId_ = 0;
    int totalCounted_ = 0;

    double dt_;
    std::chrono::steady_clock::time_point dtLastTime_;
};
