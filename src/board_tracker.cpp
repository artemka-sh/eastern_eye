#include "board_tracker.hpp"
#include <algorithm>
#include <print>

BoardTracker::BoardTracker() : dtLastTime_(std::chrono::steady_clock::now()) {}

void BoardTracker::update(const std::vector<DetectedBoard>& detections) {
    auto now = std::chrono::steady_clock::now();
    dt_ = std::chrono::duration<double>(now - dtLastTime_).count();
    dtLastTime_ = now;

    // Слепое предсказание для всех активных треков
    for (auto& track : activeTracks_) {
        track.currentPredBox = track.tracker.predict(dt_);
    }

    if (!detections.empty()) {
        std::vector<bool> detectionMatched(detections.size(), false);

        for (auto& track : activeTracks_) {
            float bestIoU = 0.0f;
            int bestIdx = -1;

            // поиск позиции которая лучше всего накладывается на предскзанную позицию
            // можно будеть заменить на венгерский алгортим, что будет лучше
            for (size_t i = 0; i < detections.size(); ++i) {
                if (detectionMatched[i]) continue;

                float iou = computeIoU(track.currentPredBox, detections[i].rBox);

                if (iou > bestIoU && iou > cfg.minIouMatch_) {
                    bestIoU = iou;
                    bestIdx = static_cast<int>(i);
                }
            }

            if (bestIdx >= 0) {
                // Корректируем Калмана реальными данными с камеры
                track.tracker.correct(detections[bestIdx].rBox);

                track.geometry = detections[bestIdx];
                track.framesLost = 0;
                track.framesSeen++;
                detectionMatched[bestIdx] = true;
            } else {
                track.framesLost++;
                // Камера не нашла доску, отдаем предсказание для плавной отрисовки инерции
                track.geometry.rBox = track.currentPredBox;
            }
        }

        // Регистрируем новые доски, которые только что въехали в кадр
        for (size_t i = 0; i < detections.size(); ++i) {
            if (!detectionMatched[i]) {
                createNewTrack(detections[i]);
            }
        }
    } else {
        // Кадры без работы детектора: доски едут чисто по математической инерции
        for (auto& track : activeTracks_) {
            track.framesLost++;
            track.geometry.rBox = track.currentPredBox;
        }
    }

    countBoards();
    cleanupLostTracks();
}

void BoardTracker::createNewTrack(const DetectedBoard& board) {
    BoardTrack track;
    track.id = nextId_++;
    track.geometry = board;
    track.tracker.create(board.rBox);

    activeTracks_.push_back(std::move(track));
}

void BoardTracker::countBoards() {
    for (auto& track : activeTracks_) {
        if (!track.counted &&
            track.getCentroid().x > cfg.countLineX_ &&
            track.getCentroid().y > cfg.countLineY_ &&
            track.framesSeen > cfg.minFramesStable_ &&
            track.framesLost == 0) {

            track.counted = true;
            totalCounted_++;
        }
    }
}

void BoardTracker::cleanupLostTracks() {
    activeTracks_.erase(
        std::remove_if(activeTracks_.begin(), activeTracks_.end(),
            [this](const BoardTrack& t) {
                // Удаляем трек, если он слишком долго не виделся камерой
                // или уехал за верхнюю границу экрана
                return t.framesLost > cfg.maxFramesLost_ || t.geometry.rBox.center.y < 100;
            }),
        activeTracks_.end()
    );
}

float BoardTracker::computeIoU(const cv::RotatedRect& a, const cv::RotatedRect& b) const {
    std::vector<cv::Point2f> intersectingRegion;
    // может быть тяжело, проверить
    int intersectionType = cv::rotatedRectangleIntersection(a, b, intersectingRegion);

    if (intersectionType == cv::INTERSECT_NONE || intersectingRegion.empty()) {
        return 0.0f;
    }

    float intersectArea = static_cast<float>(cv::contourArea(intersectingRegion));
    float areaA = a.size.area();
    float areaB = b.size.area();
    float unionArea = areaA + areaB - intersectArea;

    return unionArea > 0 ? intersectArea / unionArea : 0.0f;
}