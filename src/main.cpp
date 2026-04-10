#include "inspection_system.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>

void onMainWindowTrakbarSlide(int pos , void* cap)
{
    cv::VideoCapture* _cap = static_cast<cv::VideoCapture*>(cap);
    _cap->set(cv::CAP_PROP_POS_FRAMES, pos);
}
void onMainWindowSpeedTrakbarSlide(int pos, void*)
{

}

int main(int argc, char** argv)
{
    cv::namedWindow("Original", cv::WINDOW_NORMAL);
    cv::namedWindow("LAB Mask", cv::WINDOW_NORMAL);

    // Путь к видео (или камера)
    std::string videoPath = argc > 1 ? argv[1] : "/dev/video10";

    cv::VideoCapture cap(videoPath);
    // cap.open();
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open video: " << videoPath << std::endl;
        return -1;
    }
    // int speed = 1;
    // try
    // {
    //     int g_slider_position = 0;
    //     uint frames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    //     cv::createTrackbar("Position", "Original", &g_slider_position, frames, onMainWindowTrakbarSlide, &cap);
    //     cv::createTrackbar("Speed", "Original", &speed, 8, onMainWindowSpeedTrakbarSlide, {});
    // }catch (std::exception &e){std::cerr << "Трекер не удалось создать: " << e.what() << std::endl;}
    // Инициализация системы
    InspectionSystem system;

    // Настройки под конвейер (можно менять)
    system.setAnalysisZone(0, INT_MAX, 0 , INT_MAX);
    //system.setAnalysisZone(23, 472, 52 , 712);  // Зона где анализируем доски
    system.setLineStopThreshold(60);   // Push через 60 секунд без движения

    cv::Mat frame;
    bool paused = false;

    std::cout << "=== Board Inspection System MVP ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - pause/resume" << std::endl;
    std::cout << "  ESC   - quit" << std::endl;
    std::cout << "  S     - save current frame" << std::endl;

    int frameNum = 0;

    while (true)
    {
        if (paused)
        {
            sleep(1);
            goto keychek;
        }

        cap >> frame; if (frame.empty()) break;
        try
        {
            int current_pos = (int)cap.get(cv::CAP_PROP_POS_FRAMES);
            cv::setTrackbarPos("Position", "Original", current_pos);
        }
        catch (cv::Exception &e){std::cout << e.what();}

        frameNum++;

        // Обработка кадра
        system.processFrame(frame);

        // Визуализация
        system.draw(frame);

        if (frameNum % 10 == 0) {
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



        cv::imshow("Original", frame);

        keychek:
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