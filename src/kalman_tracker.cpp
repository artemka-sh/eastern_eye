//
// Created by artem on 14.06.2026.
//

#include "kalman_tracker.hpp"

void KalmanTracker::create(cv::RotatedRect box)
{
    // 8 параметров в памяти: cx, cy, w, h, angle, vx, vy, v_angle
    // 5 параметров с камеры: cx, cy, w, h, angle
    kf_ = cv::KalmanFilter(8, 5, 0, CV_32F);

    // переменные в матрице идут ровно с измеряемыми данными, по этому можно всё через единичные матрицы
    cv::setIdentity(kf_.transitionMatrix);
    cv::setIdentity(kf_.measurementMatrix);

    // настройки монолитности трекера
    cv::setIdentity(kf_.errorCovPost, cv::Scalar::all(1.0f));
    cv::setIdentity(kf_.processNoiseCov, cv::Scalar::all(1e-2f));
    cv::setIdentity(kf_.measurementNoiseCov, cv::Scalar::all(1e-1f));

    // Установка старовых данных
    kf_.statePost.at<float>(0) = box.center.x;
    kf_.statePost.at<float>(1) = box.center.y;
    kf_.statePost.at<float>(2) = box.size.width;
    kf_.statePost.at<float>(3) = box.size.height;
    kf_.statePost.at<float>(4) = box.angle;
    // Стартовые скорости
    kf_.statePost.at<float>(5) = 0.0f; // vx
    kf_.statePost.at<float>(6) = 0.0f; // vy
    kf_.statePost.at<float>(7) = 0.0f; // va (скорость вращения)
}

cv::RotatedRect KalmanTracker::predict(double dt)
{
    // связывание координаты со скоростями через время dt
    // индекс 0(cx) зависит от 5(vx)
    // индекс 1(cy) зависит от 6(vy)
    // индекс 4(angle) зависит от 7(va)
    kf_.transitionMatrix.at<float>(0, 5) = static_cast<float>(dt);
    kf_.transitionMatrix.at<float>(1, 6) = static_cast<float>(dt);
    kf_.transitionMatrix.at<float>(4, 7) = static_cast<float>(dt);

    // шаг слепого предсказания
    cv::Mat pred = kf_.predict();

    return cv::RotatedRect(
        cv::Point2f(pred.at<float>(0), pred.at<float>(1)), // Центр
        cv::Size2f(pred.at<float>(2), pred.at<float>(3)),  // Размеры (ШхВ)
        pred.at<float>(4)                                  // Угол
    );
}

void KalmanTracker::correct(cv::RotatedRect box)
{
    // защита резкого изменения показаня угла
    float predAngle = kf_.statePre.at<float>(4);
    float detAngle = box.angle;

    // Нормализуем угол детекции, чтобы он был максимально близок к предсказанному.
    while (detAngle - predAngle > 90.0f)  detAngle -= 180.0f;
    while (detAngle - predAngle < -90.0f) detAngle += 180.0f;

    cv::Mat meas = (cv::Mat_<float>(5, 1) <<
        box.center.x,
        box.center.y,
        box.size.width,
        box.size.height,
        detAngle
    );

    kf_.correct(meas);
}