
#include "board_tracker.hpp"
#include <algorithm>
#include <print>

BoardTracker::BoardTracker() = default;

void BoardTracker::update(const cv::Mat& frame, const std::vector<DetectedBoard>& detections) {
    // 1. Обновляем координаты всех треков с помощью KCF
    for (auto& track : activeTracks_) {
        cv::Rect bbox = track.getBoundingBox(); // Берем текущий AABB

        // Трекер пытается найти доску на новом кадре
        bool success = track.tracker->update(frame, bbox);

        if (success) {
            // Если KCF нашел доску, мы сдвигаем математический центр нашей геометрии
            track.geometry.rBox.center = cv::Point2f(bbox.x + bbox.width / 2.0f, bbox.y + bbox.height / 2.0f);
            // track.geometry.rBox.size = cv::Size2f(bbox.width, bbox.height);

            track.framesSeen++;
        } else {
            track.framesLost++;
        }
    }

    // 2. Если в этом кадре отработал Детектор, матчим его результаты с треками
    if (!detections.empty()) {
        std::vector<bool> detectionMatched(detections.size(), false);
        matchDetectionsToTracks(frame, detections, detectionMatched);
        activeTracks_.erase(
        std::remove_if(activeTracks_.begin(), activeTracks_.end(),
            [&](const BoardTrack& t) { return toRemove_.count(t.id); }),
        activeTracks_.end()
       );
        // 3. Создаём новые треки для детекций, которые ни с кем не совпали
        for (size_t i = 0; i < detections.size(); ++i) {
            if (!detectionMatched[i]) {
                createNewTrack(frame, detections[i]);
            }
        }
    }

    // 4. Подсчёт досок, пересекших линию
    countBoards();

    // 5. Удаление потерянных и уехавших треков
    cleanupLostTracks();
}

//TODO: эта часть кода не используется

void BoardTracker::matchDetectionsToTracks(const cv::Mat& frame,
                                           const std::vector<DetectedBoard>& detections,
                                           std::vector<bool>& matched) {
    for (auto& track : activeTracks_) {
        if (track.framesLost > maxFramesLost_)
        {
            std::print("Максимальное количество кадров превышено {}", track.id);
        }
        float bestIoU = 0.0f;
        int bestIdx = -1;

        cv::Rect trackBbox = track.getBoundingBox();

        // Ищем детекцию, которая лучше всего накладывается на текущий трек
        for (size_t i = 0; i < detections.size(); ++i) {
            if (matched[i]) continue;

            cv::Rect detBbox = detections[i].rBox.boundingRect();
            float iou = computeIoU(trackBbox, detBbox);

            if (iou > bestIoU && iou > minIouMatch_) {
                bestIoU = iou;
                bestIdx = static_cast<int>(i);
            }
        }

        // БИНГО! Мы сопоставили трек и свежую детекцию
        if (bestIdx >= 0) {
            // 1. ПОЛНОСТЬЮ ОБНОВЛЯЕМ ГЕОМЕТРИЮ (теперь у нас свежие, точные углы!)
            track.geometry = detections[bestIdx];
            track.framesLost = 0;
            // 2. Перезапускаем KCF с новой точной рамки, чтобы он не "уплывал" со временем
            track.tracker = cv::TrackerKCF::create();
            track.tracker->init(frame, track.getBoundingBox());
            matched[bestIdx] = true;
            for (const auto& other : activeTracks_)
            {
                if (other.id != track.id &&
                    computeIoU(other.getBoundingBox(), track.getBoundingBox()) > minIouMatch_)
                    {
                        toRemove_.insert(other.id);
                    }
            }
        }
    }
}

void BoardTracker::createNewTrack(const cv::Mat& frame, const DetectedBoard& board) {
    BoardTrack track;
    track.id = nextId_++;

    // Сохраняем всю прецизионную геометрию сразу при рождении трека
    track.geometry = board;

    track.tracker = cv::TrackerKCF::create();
    track.tracker->init(frame, track.getBoundingBox());

    activeTracks_.push_back(std::move(track));
}

void BoardTracker::countBoards() {
    for (auto& track : activeTracks_) {
        // Заменили track.centroid.x на track.getCentroid().x
        if (!track.counted &&
            track.getCentroid().x > countLineX_ && track.getCentroid().y > countLineY_ &&
            track.framesSeen > minFramesStable_ &&
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
                //удалить если потерян долго или слишком близко к верху (куда уходят доски)
                return t.framesLost > maxFramesLost_ || t.getBoundingBox().y + t.getBoundingBox().height/3 < 0  + 100;
            }),
        activeTracks_.end()
    );
}

float BoardTracker::computeIoU(const cv::Rect& a, const cv::Rect& b) const {
    cv::Rect intersection = a & b;
    if (intersection.area() == 0) return 0.0f;

    float intersectArea = static_cast<float>(intersection.area());
    float unionArea = static_cast<float>(a.area() + b.area()) - intersectArea;

    return unionArea > 0 ? intersectArea / unionArea : 0.0f;
}