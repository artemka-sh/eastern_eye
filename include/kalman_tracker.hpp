//
// Created by artem on 14.06.2026.
//

#ifndef EASTERN_EYE_KALMAN_TRACKER_HPP
#define EASTERN_EYE_KALMAN_TRACKER_HPP
#include <opencv2/video/tracking.hpp>
#include <opencv2/core.hpp>

class KalmanTracker
{
    cv::KalmanFilter kf_;
public:
    explicit KalmanTracker() = default;
    void create(cv::RotatedRect initBox);
    cv::RotatedRect predict(double dt);
    void correct(cv::RotatedRect box);
};


#endif //EASTERN_EYE_KALMAN_TRACKER_HPP