#include "inspection_system.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>

int main(int argc, char** argv) {
    // Путь к видео (или камера)
    std::string videoPath = argc > 1 ? argv[1] : "res/conveyor.mp4";

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open video: " << videoPath << std::endl;
        return -1;
    }

    // Инициализация системы
    InspectionSystem system;

    // Настройки под конвейер (можно менять)
    system.setAnalysisZone(0, 480);  // Зона где анализируем доски
    system.setLineStopThreshold(60);   // Push через 60 секунд без движения

    cv::Mat frame;
    bool paused = false;

    std::cout << "=== Board Inspection System MVP ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - pause/resume" << std::endl;
    std::cout << "  ESC   - quit" << std::endl;
    std::cout << "  S     - save current frame" << std::endl;

    int frameNum = 0;

    while (true) {
        if (!paused) {
            if (!cap.read(frame)) {
                std::cout << "End of video" << std::endl;
                break;
            }

            frameNum++;

            // Обработка кадра
            system.processFrame(frame);

            // Визуализация
            system.draw(frame);

            // Статистика в консоль (каждые 30 кадров)
            if (frameNum % 30 == 0) {
                const auto& stats = system.getStats();
                std::cout << "Frame " << frameNum
                         << " | Counted: " << stats.totalCounted;

                for (const auto& [cat, count] : stats.categoryCounts) {
                    std::cout << " | " << cat << ": " << count;
                }

                if (system.isLineStopped()) {
                    std::cout << " | WARNING: Line stopped!";
                }

                std::cout << std::endl;
            }
        }

        cv::imshow("Board Inspection", frame);

        char key = cv::waitKey(paused ? 0 : 30);

        if (key == 27) {  // ESC
            break;
        } else if (key == ' ') {  // SPACE
            paused = !paused;
            std::cout << (paused ? "PAUSED" : "RESUMED") << std::endl;
        } else if (key == 's' || key == 'S') {
            std::string filename = "screenshot_" + std::to_string(frameNum) + ".jpg";
            cv::imwrite(filename, frame);
            std::cout << "Saved: " << filename << std::endl;
        }
    }

    // Финальная статистика
    const auto& stats = system.getStats();
    std::cout << "\n=== Final Statistics ===" << std::endl;
    std::cout << "Total boards counted: " << stats.totalCounted << std::endl;

    for (const auto& [category, count] : stats.categoryCounts) {
        float percentage = stats.totalCounted > 0
            ? (100.0f * count / stats.totalCounted)
            : 0.0f;
        std::cout << category << ": " << count
                 << " (" << percentage << "%)" << std::endl;
    }

    return 0;
}