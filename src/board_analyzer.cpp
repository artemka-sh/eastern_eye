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
    cv::Mat maskROI = getROImask(boardROI);
    boardROI.setTo(cv::Scalar(0, 0, 0), maskROI);

    cv::imshow("Board ROI", boardROI);

    track.avgColor = analyzeColor(boardROI);

    track.defects = detectDefects(boardROI);

    // track.category = classifyBoard(track.avgColor, track.defectRatio);


    drawDefects(boardROI, track.defects);
    cv::imshow("Board defects", boardROI);

    track.analyzed = true;
}

cv::Mat BoardAnalyzer::getROImatrix(const cv::RotatedRect& rBox, const cv::Mat& frame) const noexcept
{
    const cv::Size dst(static_cast<int>(std::round(rBox.size.width)),
                       static_cast<int>(std::round(rBox.size.height)));


    cv::Mat warpingData = cv::getRotationMatrix2D(rBox.center, rBox.angle, 1.0);
    //после нужно сдвинуть изображение в центр новой матрицы, перенести в другую систему координат
    warpingData.at<double>(0, 2) += dst.width  * 0.5 - rBox.center.x;
    warpingData.at<double>(1, 2) += dst.height * 0.5 - rBox.center.y;

    cv::Mat board;
    cv::warpAffine(frame, board, warpingData, dst, cv::INTER_LINEAR,
                   cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    if (board.size().width > board.size().height) // вернуть в исходное вертикальное полоежине после
        cv::rotate(board, board, cv::ROTATE_90_COUNTERCLOCKWISE);
    return board;
}

inline cv::Mat BoardAnalyzer::getROImask(cv::Mat &boardROI) const noexcept
{
    cv::Mat hsv, greenMask;
    cv::cvtColor(boardROI, hsv, cv::COLOR_BGR2HSV);

    cv::inRange(hsv,
        cv::Scalar(cfg.chromakeyMin_H_S_V[0], cfg.chromakeyMin_H_S_V[1], cfg.chromakeyMin_H_S_V[2]),
        cv::Scalar(cfg.chromakeyMax_H_S_V[0], cfg.chromakeyMax_H_S_V[1], cfg.chromakeyMax_H_S_V[2]),
        greenMask);

    return greenMask;
}

std::vector<std::pair<LetterboxInfo, cv::Mat>> BoardAnalyzer::getROISections(const cv::Mat& boardROI)
{
    const int srcW = boardROI.cols;
    const int srcH = boardROI.rows;
    const int section_sizeXY = srcW;

    // расчёт количества с учётом процента перекрытия
    const int overlap = static_cast<int>(section_sizeXY * cfg.SECTION_OVERLAP_PERCENT);
    const int step_initial = section_sizeXY - overlap;

    int n = srcH / step_initial;
    int mod = srcH % step_initial;
    float remainder = static_cast<float>(mod) / step_initial;
    if (remainder >= cfg.TOLERANCE_FOR_TILES) ++n;
    n = std::max(1, n);

    const int tile_h = (srcH + (n - 1) * overlap) / n;
    const int step = tile_h - overlap;

    std::vector<std::pair<LetterboxInfo, cv::Mat>> sections;
    sections.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        int start_y = (i == n - 1) ? (srcH - tile_h) : (i * step);

        LetterboxInfo meta;
        meta.y_offset = start_y;
        cv::Mat crop = boardROI(cv::Rect(0, start_y, srcW, tile_h));
        sections.emplace_back(meta, crop);
    }
    return sections;
}

std::vector<Defect> BoardAnalyzer::detectDefects(const cv::Mat& boardROI)
{
    if (boardROI.empty())
        return {};

    std::vector<std::pair<LetterboxInfo, cv::Mat>> sections = getROISections(boardROI);
    //данные ниже собираются функцией постпроцессинга для того,
    //чтобы потом сложить их и найти все дефекты разом функцией отчистки
    RawDetections all;
    all.roiSize = boardROI.size();
    for (auto& section : sections)
    {
        LetterboxInfo& info = section.first;
        cv::Mat& crop = section.second;

        cv::Mat blob = preprocess(crop, info);
        net.setInput(blob);

        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());
        if (outputs.empty() || outputs[0].empty())
            continue;

        RawDetections raw = postprocess(outputs[0], boardROI.size(), info);
        all.confidences.append_range(raw.confidences);
        all.boxes.append_range(raw.boxes);
        all.classIds.append_range(raw.classIds);
    }
    return getDefects_withRemoveBoxes(all);
}

cv::Mat BoardAnalyzer::preprocess(const cv::Mat& boardROI, LetterboxInfo& info) const
{
    // Letterbox = ресайз с сохранением соотношения сторон + паддинг серым (114).
    // Без него на не-квадратных кропах боксы поплывут.
    const int netSize = static_cast<int>(cfg.INPUT_SIZE); //используется и INPUT_SIZE, что удобнее
    const int srcW = boardROI.cols;
    const int srcH = boardROI.rows;

    info.scale = std::min(cfg.INPUT_SIZE / srcW,
                          cfg.INPUT_SIZE / srcH);
    const int newW = static_cast<int>(std::round(srcW * info.scale));
    const int newH = static_cast<int>(std::round(srcH * info.scale));
    info.padX = (netSize - newW) / 2;
    info.padY = (netSize - newH) / 2;

    cv::Mat resized;
    // CUBIC при апскейле — резче тонкие трещины; AREA при даунскейле — без ореолов вокруг линий
    const int interp = (info.scale >= 1.0f) ? cv::INTER_CUBIC : cv::INTER_AREA;
    cv::resize(boardROI, resized, cv::Size(newW, newH), 0, 0, interp);
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

RawDetections BoardAnalyzer::postprocess(
    const cv::Mat& rawOutput, cv::Size roiSize,
    const LetterboxInfo& info) const
{
    // Форма выхода yolov8 detect: [1, 4+numClasses, N], N=8400 для 640.
    if (rawOutput.dims != 3 || rawOutput.size[0] != 1)
        throw std::string("фатальная ошибка - код не готов к нескольким изображениям");

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

    for (int i = 0; i < out.rows; ++i)
    {
        const float* row = out.ptr<float>(i);
        std::span<const float> scores(row + 4, numClasses);

        auto maxIt = std::ranges::max_element(scores);
        const float maxClassScore = *maxIt;

        if (maxClassScore < cfg.SCORE_THRESHOLD)
            continue;

        const int classId = static_cast<int>(maxIt - scores.begin());

        const float cx = row[0], cy = row[1], w = row[2], h = row[3];

        const float left = (cx - w * 0.5f - info.padX) / info.scale;
        const float top  = (cy - h * 0.5f - info.padY) / info.scale + info.y_offset;
        const float boxW = w / info.scale;
        const float boxH = h / info.scale;

        boxes.emplace_back(static_cast<int>(left), static_cast<int>(top),
                           static_cast<int>(boxW), static_cast<int>(boxH));
        confidences.push_back(maxClassScore);
        classIds.push_back(classId);
    }

    // ещё нужно сделать подавление немаксимумов (NMS — non-maximum suppression):
    // и убирает дублирующиеся боксы на одном объекте и это делает функция которая эта ловит
    return RawDetections{ std::move(confidences), std::move(boxes), std::move(classIds), roiSize };
}


std::vector<Defect> BoardAnalyzer::getDefects_withRemoveBoxes(const RawDetections& raw) const
{
    std::vector<int> keep;
    cv::dnn::NMSBoxesBatched(raw.boxes, raw.confidences, raw.classIds,
                             cfg.SCORE_THRESHOLD, cfg.NMS_THRESHOLD, keep);

    const cv::Rect roiBounds(cv::Point(), raw.roiSize);
    std::vector<Defect> defects;
    defects.reserve(keep.size());
    for (int idx : keep)
    {
        Defect d;
        d.bbox     = raw.boxes[idx] & roiBounds;
        d.severity = raw.confidences[idx];
        d.classIdx = raw.classIds[idx];
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
        std::string name = "unknown";
        cv::Scalar  color = Color::RED;

        if (d.classIdx >= 0 &&
            d.classIdx < static_cast<int>(cfg.defectClasses.size()))
        {
            const auto& cls = cfg.defectClasses[d.classIdx];
            name  = cls.name;
            color = toScalar(cls.colorBGR);
        }

        cv::rectangle(image, d.bbox, color, cfg.THICKNESS * 2);
        const std::string label = cv::format("%s: %.2f", name.c_str(), d.severity);
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