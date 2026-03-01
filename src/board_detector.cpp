#include "board_detector.hpp"
#include <cmath>
#include <iostream>

BoardDetector::BoardDetector() {
    // Оставляем ядро 7x7 для морфологии
    morphKernel_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
}

std::vector<DetectedBoard> BoardDetector::detect(const cv::Mat& frame) {
    std::vector<DetectedBoard> boards;

    // 1. Защита от пустых/битых кадров камеры
    if (frame.empty() || frame.cols <= 10 || frame.rows <= 10) {
        std::cerr << "Предупреждение: получен пустой или битый кадр!" << std::endl;
        return {};
    }

    // 1. ПЕРЕВОД В LAB И ВЫДЕЛЕНИЕ МАСКИ
    cv::Mat lab, gray, mask;
    cv::cvtColor(frame, lab, cv::COLOR_BGR2Lab);
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY); // Понадобится позже для субпикселей

    std::vector<cv::Mat> channels;
    cv::split(lab, channels);

    // channels[1] - это канал 'a'. Всё, что зеленее нейтрали, отсекается.
    // Если доски не выделяются, поиграйся с labAThreshold_ (по умолчанию 130)
    cv::threshold(channels[1], mask, labAThreshold_, 255, cv::THRESH_BINARY);

    // 2. МОРФОЛОГИЯ ДЛЯ ОЧИСТКИ ШУМА
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, morphKernel_);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, morphKernel_);

    // Показываем маску для отладки (убедись, что доски белые, а конвейер черный)
    cv::imshow("LAB Mask", mask);

    // 3. ПОИСК КОНТУРОВ
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 4. АНАЛИЗ КАЖДОЙ ДОСКИ
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);

        // Фильтр по площади
        if (area < minArea_ || area > maxArea_) continue;

        // Получаем повернутый прямоугольник
        cv::RotatedRect rBox = cv::minAreaRect(contour);

        // Фильтр по форме. Так как ширина и высота могут меняться местами из-за угла,
        // считаем соотношение большей стороны к меньшей
        float w = rBox.size.width;
        float h = rBox.size.height;
        float aspectRatio = (w > h) ? (w / h) : (h / w);

        if (aspectRatio < minAspectRatio_ || aspectRatio > maxAspectRatio_) continue;

        // Фильтр по позиции на конвейере
        if (rBox.center.y < minY_ || rBox.center.y > maxY_) continue;

        // --- НАЧИНАЕТСЯ ВЫЧИСЛЕНИЕ ТОЧНОЙ ГЕОМЕТРИИ ---
        DetectedBoard board;
        board.rBox = rBox;
        board.isGeometryValid = false;

        std::vector<cv::Point> approx;
        // Эпсилон: чем больше, тем сильнее упрощается контур. 2% от периметра - хороший старт.
        double epsilon = 0.02 * cv::arcLength(contour, true);
        cv::approxPolyDP(contour, approx, epsilon, true);

        // Нам нужно вытащить 4 угла. Если аппроксимация дала ровно 4:
        if (approx.size() == 4) {
            for (const auto& p : approx) {
                board.corners.push_back(cv::Point2f(p.x, p.y));
            }
        } else {
            // Фолбэк (запасной план): если края сильно изжеваны и получилось 5 или 3 точки,
            // просто берем 4 идеальных угла от нашего повернутого прямоугольника minAreaRect
            cv::Point2f rectPts[4];
            rBox.points(rectPts);
            for (int i = 0; i < 4; i++) {
                board.corners.push_back(rectPts[i]);
            }
        }

        // 5. СУБПИКСЕЛЬНОЕ УТОЧНЕНИЕ УГЛОВ (Выжимаем точность)
        cv::Size winSize(11, 11); // Окно поиска 11x11 пикселей
        cv::Size zeroZone(-1, -1);
        cv::TermCriteria criteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 40, 0.001);


        // --- УЛУЧШЕННАЯ ЗАЩИТА ---
        // Вместо 0 и cols-1, отступаем на размер окна поиска (winSize)
        float margin = 12.0f;
        for (auto& pt : board.corners) {
            pt.x = std::max(margin, std::min(pt.x, static_cast<float>(gray.cols - margin)));
            pt.y = std::max(margin, std::min(pt.y, static_cast<float>(gray.rows - margin)));
        }

        // Функция "подтягивает" наши найденные углы к реальным резким перепадам яркости
        cv::cornerSubPix(gray, board.corners, winSize, zeroZone, criteria);

        // 6. ВЫЧИСЛЕНИЕ 4-Х УГЛОВ СПИЛА В ГРАДУСАХ
        for (int i = 0; i < 4; i++) {
            cv::Point2f pPrev = board.corners[(i + 3) % 4];
            cv::Point2f pCurrent = board.corners[i];
            cv::Point2f pNext = board.corners[(i + 1) % 4];

            double angle = calculateAngle(pPrev, pCurrent, pNext);
            board.angles.push_back(angle);
        }

        board.isGeometryValid = true;
        boards.push_back(board);
    }

    return boards;
}

double BoardDetector::calculateAngle(cv::Point2f A, cv::Point2f B, cv::Point2f C) {
    // Векторы BA и BC
    cv::Point2f BA = {A.x - B.x, A.y - B.y};
    cv::Point2f BC = {C.x - B.x, C.y - B.y};

    // Скалярное произведение
    double dotProduct = BA.x * BC.x + BA.y * BC.y;

    // Длины векторов
    double magBA = std::sqrt(BA.x * BA.x + BA.y * BA.y);
    double magBC = std::sqrt(BC.x * BC.x + BC.y * BC.y);

    if (magBA == 0 || magBC == 0) return 0.0;

    // Косинус угла
    double cosTheta = dotProduct / (magBA * magBC);

    // Защита от выхода за пределы [-1, 1] из-за погрешностей float
    cosTheta = std::max(-1.0, std::min(1.0, cosTheta));

    // Возвращаем угол в градусах
    return std::acos(cosTheta) * 180.0 / CV_PI;
}