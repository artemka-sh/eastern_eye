#include "board_tracker.hpp"
#include <opencv2/video/tracking.hpp> // Для cv::Tracker
#include <opencv2/tracking.hpp>       // Для cv::TrackerKCF
#include <algorithm>

BoardTracker::BoardTracker() = default;

void BoardTracker::update(const cv::Mat& frame, const std::vector<cv::Rect>& detections) {
    // 1. Обновляем существующие трекеры
    for (auto& track : activeTracks_) {
        // Конвертируем Rect в Rect2d для API трекера
        cv::Rect2i bboxDouble = track.bbox; /////////////////////////////////
        bool success = track.tracker->update(frame, bboxDouble);

        if (success) {
            track.bbox = bboxDouble;  // Конвертируем обратно
            track.framesSeen++;
            track.framesLost = 0;
            track.centroid = getCentroid(track.bbox);
        } else {
            track.framesLost++;
        }
    }

    // 2. Матчим новые детекции с существующими треками
    std::vector<bool> detectionMatched(detections.size(), false);
    matchDetectionsToTracks(frame, detections, detectionMatched);

    // 3. Создаём новые треки
    for (size_t i = 0; i < detections.size(); ++i) {
        if (!detectionMatched[i]) {
            createNewTrack(frame, detections[i]);
        }
    }

    // 4. Подсчёт досок
    countBoards();

    // 5. Очистка
    cleanupLostTracks(frame.cols);
}

void BoardTracker::matchDetectionsToTracks(const cv::Mat& frame,
                                           const std::vector<cv::Rect>& detections,
                                           std::vector<bool>& matched) {
    for (auto& track : activeTracks_) {
        if (track.framesLost > 5)
            continue;

        float bestIoU = 0.0f;
        int bestIdx = -1;

        for (size_t i = 0; i < detections.size(); ++i) {
            if (matched[i])
                continue;

            float iou = computeIoU(track.bbox, detections[i]);

            if (iou > bestIoU && iou > minIouMatch_) {
                bestIoU = iou;
                bestIdx = static_cast<int>(i);
            }
        }

        if (bestIdx >= 0) {
            track.tracker = cv::TrackerKCF::create();
            cv::Rect2d bboxDouble = detections[bestIdx];
            track.tracker->init(frame, bboxDouble);
            track.bbox = detections[bestIdx];
            track.centroid = getCentroid(detections[bestIdx]);
            track.framesLost = 0;
            matched[bestIdx] = true;
        }
    }
}

void BoardTracker::createNewTrack(const cv::Mat& frame, const cv::Rect& bbox) {
    BoardTrack track;
    track.id = nextId_++;
    track.bbox = bbox;
    track.centroid = getCentroid(bbox);
    track.tracker = cv::TrackerKCF::create();

    cv::Rect2d bboxDouble = bbox;
    track.tracker->init(frame, bboxDouble);

    activeTracks_.push_back(std::move(track));
}

void BoardTracker::countBoards() {
    for (auto& track : activeTracks_) {
        if (!track.counted && 
            track.centroid.x > countLineX_ &&
            track.framesSeen > minFramesStable_ &&
            track.framesLost == 0) {
            
            track.counted = true;
            totalCounted_++;
        }
    }
}

void BoardTracker::cleanupLostTracks(int frameWidth) {
    activeTracks_.erase(
        std::remove_if(activeTracks_.begin(), activeTracks_.end(),
            [this, frameWidth](const BoardTrack& t) {
                return t.framesLost > maxFramesLost_ || t.bbox.x > frameWidth + 100;
            }),
        activeTracks_.end()
    );
}

float BoardTracker::computeIoU(const cv::Rect& a, const cv::Rect& b) const {
    cv::Rect intersection = a & b;
    if (intersection.area() == 0)
        return 0.0f;
    
    float intersectArea = static_cast<float>(intersection.area());
    float unionArea = static_cast<float>(a.area() + b.area()) - intersectArea;
    
    return unionArea > 0 ? intersectArea / unionArea : 0.0f;
}

cv::Point2f BoardTracker::getCentroid(const cv::Rect& bbox) const {
    return cv::Point2f(bbox.x + bbox.width / 2.0f, bbox.y + bbox.height / 2.0f);
}