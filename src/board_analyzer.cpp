#include "board_analyzer.hpp"

BoardAnalyzer::BoardAnalyzer() = default;

void BoardAnalyzer::analyze(const cv::Mat& frame, BoardTrack& track) {
    return; //TODO АНАЛИЗА ДОСОК НЕТ
    if (track.analyzed)
        return;

    cv::Rect bbox = track.getBoundingBox() & cv::Rect(0, 0, frame.cols, frame.rows);
    if (bbox.width < 4 || bbox.height < 4)
        return;
    cv::Mat boardROI = frame(bbox);

    // Анализ цвета
    track.avgColor = analyzeColor(boardROI);
    
    // Детекция дефектов (примитивная)
    track.defects = detectDefectsSimple(boardROI);
    
    // Подсчёт соотношения дефектов
    int defectPixels = 0;
    for (const auto& defect : track.defects) {
        defectPixels += defect.bbox.area();
    }
    track.defectRatio = static_cast<float>(defectPixels) / (boardROI.cols * boardROI.rows);
    
    // Классификация
    float lightness = track.avgColor[0];  // L канал в LAB
    track.category = classifyBoard(track.avgColor, track.defectRatio);
    
    track.analyzed = true;
}

cv::Scalar BoardAnalyzer::analyzeColor(const cv::Mat& boardROI) {
    cv::Mat lab;
    cv::cvtColor(boardROI, lab, cv::COLOR_BGR2Lab);
    return cv::mean(lab);
}

std::vector<Defect> BoardAnalyzer::detectDefectsSimple(const cv::Mat& boardROI) {
    // Простая пороговая сегментация для MVP
    // TODO: Заменить на YOLOv8 детектор дефектов
    
    cv::Mat gray;
    cv::cvtColor(boardROI, gray, cv::COLOR_BGR2GRAY);
    
    // Находим тёмные участки (сучки, трещины)
    cv::Mat darkMask;
    cv::threshold(gray, darkMask, 80, 255, cv::THRESH_BINARY_INV);
    
    // Морфология для объединения близких дефектов
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(darkMask, darkMask, cv::MORPH_CLOSE, kernel);
    
    // Поиск контуров дефектов
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(darkMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    std::vector<Defect> defects;
    
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        
        // Фильтруем слишком мелкие (шум)
        if (area < 50)
            continue;
        
        Defect defect;
        defect.bbox = cv::boundingRect(contour);
        defect.severity = std::min(1.0f, static_cast<float>(area) / 1000.0f);
        defect.type = "dark_area";  // В DL версии будет: knot, sapwood, crack, etc
        
        defects.push_back(defect);
    }
    
    return defects;
}

std::string BoardAnalyzer::classifyBoard(const cv::Scalar& avgColor, float defectRatio) {
    float lightness = avgColor[0];  // L в LAB (0-100)
    
    // Классификация по стандартам предприятия
    if (lightness >= cfg.gradeA_minLightness_ && defectRatio <= cfg.gradeA_maxDefectRatio_) {
        return "Grade_A";
    } else if (lightness >= cfg.gradeB_minLightness_ && defectRatio <= cfg.gradeB_maxDefectRatio_) {
        return "Grade_B";
    } else {
        return "Grade_C";
    }
}