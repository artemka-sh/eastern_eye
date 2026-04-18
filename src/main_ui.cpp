#include "main_ui.hpp"
#include <opencv2/highgui.hpp>
#include <print>

static void onPositionTrackbar(int pos, void* cap)
{
    static_cast<cv::VideoCapture*>(cap)->set(cv::CAP_PROP_POS_FRAMES, pos);
}

static void onSpeedTrackbar(int pos, void* speedPtr)
{
    *static_cast<int*>(speedPtr) = std::max(1, pos);
}

void setupTrackbars(cv::VideoCapture& cap, int& speed)
{
    try
    {
        int frames = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
        int pos = 0;
        cv::createTrackbar("Position", "Original", &pos, frames, onPositionTrackbar, &cap);
        cv::createTrackbar("Speed", "Original", &speed, 8, onSpeedTrackbar, &speed);
    }
    catch (...) {}
}

void updatePositionTrackbar(cv::VideoCapture& cap)
{
    try
    {
        cv::setTrackbarPos("Position", "Original", (int)cap.get(cv::CAP_PROP_POS_FRAMES));
    }
    catch (...) {}
}

void printControls()
{
    std::println("=== Система инспекции досок ===");
    std::println("  ПРОБЕЛ    - пауза/продолжить");
    std::println("  ESC       - выход");
    std::println("  S         - сохранить кадр");
    std::println("  Скорость 1-8 - пропускать каждый N-й кадр (1 = нормально)");
}

void printFrameStats(int frameNum, const SystemStats& stats, bool lineStopped)
{
    std::string line = std::format("Кадр {} | Подсчитано: {}", frameNum, stats.totalCounted);
    for (const auto& [cat, count] : stats.categoryCounts)
        line += std::format(" | {}: {}", cat, count);
    if (lineStopped)
        line += " | ВНИМАНИЕ: конвейер остановлен!";
    std::println("{}", line);
}
