#include "inspection_system.hpp"
#include "dashboard_server.hpp"
#include "main_ui.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <csignal>
#include <atomic>
#include <print>

std::atomic<bool> keepRunning(true);

void signalHandler(int signum) {
    std::println("\n[!] Перехвачен сигнал {}  (Ctrl+C) или kill. Завершаем работу...", signum);
    keepRunning = false;
}

int main(int argc, char** argv)
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::string videoPath = argc > 1 ? argv[1] : "/dev/video10";
    std::println("IPP: {}", cv::ipp::useIPP());

    cv::namedWindow("Original", cv::WINDOW_NORMAL);
    cv::namedWindow("LAB Mask", cv::WINDOW_NORMAL);

    InspectionSystem system;
    system.loadConfig();

    SharedState state;
    DashboardServer server(state,
        [&system]{ return system.getConfig(); },
        [&system](const AppConfig& cfg){ system.applyConfig(cfg); },
        [&system]{ return system.getStatsSnapshot(); }
    );
    server.start();

    cv::VideoCapture cap(videoPath);

    int speed = 1;
    setupTrackbars(cap, speed);
    printControls();

    cv::Mat frame;
    bool paused = false;
    int frameNum = 0;

    while (keepRunning)
    {
        if (paused)
        {
            sleep(1);
            goto keychek;
        }

        cap >> frame;
        if (frame.empty())
        {
            std::println(stderr, "Попытка подключения сигнала...");
            cap.open(videoPath);
            std::println("Резульат: {}", cap.isOpened() ? "видео кажется появилось" : "безрезультатно");
            goto keychek;
        }

        frameNum++;
        updatePositionTrackbar(cap);

        if (speed < 2 || frameNum % speed == 0)
        {
            system.processFrame(frame);
            system.draw(frame);
            cv::imshow("Original", frame);
        }


        keychek:
        char key = cv::waitKey(paused ? 0 : 30);

        if (key == 27)
        {
            break;
        }
        else if (key == 'i' || key == 'I')
        {
            printFrameStats(frameNum, system.getStats(), system.isLineStopped());
        }
        else if (key == ' ')
        {
            paused = !paused;
            std::println("{}", paused ? "ПАУЗА" : "ПРОДОЛЖЕНИЕ");
        }
        else if (key == 's' || key == 'S')
        {
            std::string filename = "screenshot_" + std::to_string(frameNum) + ".jpg";
            cv::imwrite(filename, frame);
            std::println("Сохранено: {}", filename);
        }
    }

    const auto& stats = system.getStats();
    std::println("\n=== Итоговая статистика ===");
    std::println("Всего подсчитано: {}", stats.totalCounted);
    for (const auto& [cat, count] : stats.categoryCounts)
    {
        float pct = stats.totalCounted > 0 ? 100.0f * count / stats.totalCounted : 0.0f;
        std::println("{}: {} ({:.1f}%)", cat, count, pct);
    }


    cap.release();
    cv::destroyAllWindows();
    cv::waitKey(1);
    return 0;
}
