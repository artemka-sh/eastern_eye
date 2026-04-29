#include "board_analyzer.hpp"

#include <opencv2/highgui.hpp>

BoardAnalyzer::BoardAnalyzer()
{
    net = cv::dnn::readNetFromONNX("res/v1original.onnx");
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
}

void BoardAnalyzer::analyze(const cv::Mat& frame, BoardTrack& track)
{
    if (track.analyzed)
        return;

    cv::RotatedRect& rBox = track.geometry.rBox;
    cv::Mat boardROI = getROImatrix(rBox, frame);
    cv::imshow("Board ROI", boardROI);

    // Анализ цвета
    // track.avgColor = analyzeColor(boardROI);

    // Детекция дефектов
    track.defects = detectDefects(boardROI);

    cv::Mat debugView = boardROI.clone();
    drawDefects(debugView, track.defects);
    cv::imshow("Board defects", debugView);

    // // Подсчёт соотношения дефектов
    // int defectPixels = 0;
    // for (const auto& defect : track.defects) {
    //     defectPixels += defect.bbox.area();
    // }
    // track.defectRatio = static_cast<float>(defectPixels) / (boardROI.cols * boardROI.rows);

    // // Классификация
    // track.category = classifyBoard(track.avgColor, track.defectRatio);

    track.analyzed = true;
}

cv::Mat BoardAnalyzer::getROImatrix(const cv::RotatedRect& rBox, const cv::Mat& frame) const noexcept
{
    cv::Mat M = cv::getRotationMatrix2D(rBox.center, rBox.angle, 1.0);
    cv::Mat rotated;
    cv::warpAffine(frame, rotated, M, frame.size(), cv::INTER_LINEAR);
    cv::Mat board;
    cv::getRectSubPix(rotated, rBox.size, rBox.center, board);
    return board;
}

std::vector<Defect> BoardAnalyzer::detectDefects(const cv::Mat& boardROI)
{
    if (boardROI.empty())
        return {};

    LetterboxInfo info;
    cv::Mat blob = preprocess(boardROI, info);

    net.setInput(blob);
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());
    if (outputs.empty())
        return {};

    return postprocess(outputs[0], boardROI.size(), info);
}

cv::Mat BoardAnalyzer::preprocess(const cv::Mat& boardROI, LetterboxInfo& info) const
{
    // Letterbox = ресайз с сохранением соотношения сторон + паддинг серым (114).
    // Без него на не-квадратных кропах боксы поплывут.
    const int netSize = static_cast<int>(cfg.INPUT_WIDTH);
    const int srcW = boardROI.cols;
    const int srcH = boardROI.rows;

    info.scale = std::min(static_cast<float>(netSize) / srcW,
                          static_cast<float>(netSize) / srcH);
    const int newW = static_cast<int>(std::round(srcW * info.scale));
    const int newH = static_cast<int>(std::round(srcH * info.scale));
    info.padX = (netSize - newW) / 2;
    info.padY = (netSize - newH) / 2;

    cv::Mat resized;
    cv::resize(boardROI, resized, cv::Size(newW, newH));
    cv::Mat letterboxed(netSize, netSize, boardROI.type(), cv::Scalar(114, 114, 114));
    resized.copyTo(letterboxed(cv::Rect(info.padX, info.padY, newW, newH)));

    // 1.0/255 переводит пиксели uint8 [0..255] в float [0..1] — yolo обучен так.
    // swapRB=true: OpenCV держит кадры в BGR, а сеть обучена на RGB.
    cv::Mat blob;
    cv::dnn::blobFromImage(letterboxed, blob, 1.0 / 255.0,
                           cv::Size(netSize, netSize),
                           cv::Scalar(), /*swapRB=*/true, /*crop=*/false);
    return blob;
}

std::vector<Defect> BoardAnalyzer::postprocess(const cv::Mat& rawOutput, cv::Size roiSize,
                                               const LetterboxInfo& info) const
{
    // Форма выхода yolov8 detect: [1, 4+numClasses, N], где N=8400 для 640.
    // Это ГЛАВНОЕ отличие от v5 (там было [1, N, 5+nc] и был objectness).
    if (rawOutput.dims != 3 || rawOutput.size[0] != 1)
        return {};

    const int dimensions = rawOutput.size[1];
    const int rows       = rawOutput.size[2];
    const int numClasses = dimensions - 4;
    if (numClasses <= 0)
        return {};

    // reshape убирает ось пакета (batch), transpose делает строки = детекциям.
    cv::Mat out = rawOutput.reshape(1, dimensions);
    cv::transpose(out, out);

    std::vector<int>      classIds;
    std::vector<float>    confidences;
    std::vector<cv::Rect> boxes;
    classIds.reserve(rows / 8);
    confidences.reserve(rows / 8);
    boxes.reserve(rows / 8);

    const float* data = reinterpret_cast<const float*>(out.data);
    for (int i = 0; i < rows; ++i, data += dimensions)
    {
        // У v8 нет objectness — confidence берём как максимум по классам.
        const float* classScores = data + 4;
        cv::Mat scores(1, numClasses, CV_32FC1, const_cast<float*>(classScores));
        cv::Point classId;
        double maxClassScore = 0.0;
        cv::minMaxLoc(scores, nullptr, &maxClassScore, nullptr, &classId);

        if (maxClassScore < cfg.SCORE_THRESHOLD)
            continue;

        // cx, cy, w, h в пикселях входа сети (0..640), не нормированные.
        // Обратное letterbox-преобразование: минус паддинг, делим на масштаб —
        // получаем координаты в системе исходного boardROI.
        const float cx = data[0];
        const float cy = data[1];
        const float w  = data[2];
        const float h  = data[3];
        const float left = (cx - w * 0.5f - info.padX) / info.scale;
        const float top  = (cy - h * 0.5f - info.padY) / info.scale;
        const float boxW = w / info.scale;
        const float boxH = h / info.scale;

        boxes.emplace_back(static_cast<int>(left), static_cast<int>(top),
                           static_cast<int>(boxW), static_cast<int>(boxH));
        confidences.push_back(static_cast<float>(maxClassScore));
        classIds.push_back(classId.x);
    }

    // Подавление немаксимумов (NMS — non-maximum suppression):
    // убирает дублирующиеся боксы на одном объекте.
    std::vector<int> keep;
    cv::dnn::NMSBoxes(boxes, confidences,
                      cfg.SCORE_THRESHOLD, cfg.NMS_THRESHOLD, keep);

    const cv::Rect roiBounds(cv::Point(), roiSize);
    std::vector<Defect> defects;
    defects.reserve(keep.size());
    for (int idx : keep)
    {
        Defect d;
        d.bbox     = boxes[idx] & roiBounds;
        d.severity = confidences[idx];
        d.type     = std::to_string(classIds[idx]);
        defects.push_back(std::move(d));
    }
    return defects;
}

std::string BoardAnalyzer::classifyBoard(const cv::Scalar& avgColor, float defectRatio)
{
    float lightness = avgColor[0];  // канал L в Lab, диапазон 0..100

    if (lightness >= cfg.gradeA_minLightness_ && defectRatio <= cfg.gradeA_maxDefectRatio_)
        return "Grade_A";
    if (lightness >= cfg.gradeB_minLightness_ && defectRatio <= cfg.gradeB_maxDefectRatio_)
        return "Grade_B";
    return "Grade_C";
}

cv::Scalar BoardAnalyzer::analyzeColor(const cv::Mat& boardROI)
{
    cv::Mat lab;
    cv::cvtColor(boardROI, lab, cv::COLOR_BGR2Lab);
    return cv::mean(lab);
}

void BoardAnalyzer::drawDefects(cv::Mat& image, const std::vector<Defect>& defects) const
{
    for (const Defect& d : defects)
    {
        cv::rectangle(image, d.bbox, Color::RED, cfg.THICKNESS * 2);
        const std::string label = cv::format("%s: %.2f", d.type.c_str(), d.severity);
        drawLabel(image, label, d.bbox.x, d.bbox.y);
    }
}

void BoardAnalyzer::drawLabel(cv::Mat& image, const std::string& label, int left, int top) const
{
    int baseLine = 0;
    cv::Size labelSize = cv::getTextSize(label, FONT_FACE, cfg.FONT_SCALE,
                                         cfg.THICKNESS, &baseLine);
    top = std::ranges::max(top, labelSize.height);

    const cv::Point tlc(left, top);
    const cv::Point brc(left + labelSize.width, top + labelSize.height + baseLine);
    cv::rectangle(image, tlc, brc, Color::BLACK, cv::FILLED);
    cv::putText(image, label, cv::Point(left, top + labelSize.height),
                FONT_FACE, cfg.FONT_SCALE, Color::YELLOW, cfg.THICKNESS);
}