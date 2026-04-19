#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <memory>
#include "system_configuration.hpp"
#include "types.hpp"

namespace httplib { class Server; }

struct SharedState
{
    std::mutex mtx;
    std::vector<uint8_t> latestJpegBuffer;
};

class DashboardServer
{
    std::unique_ptr<httplib::Server> server_;
    std::string indexHtml_;
    std::thread serverThread_;
    SharedState& state_;
    std::function<AppConfig()> onGetConfig_;
    std::function<void(const AppConfig&)> onSetConfig_;
    std::function<StatsSnapshot()> onGetStats_;

public:
    DashboardServer(SharedState& state,
                    std::function<AppConfig()> onGetConfig,
                    std::function<void(const AppConfig&)> onSetConfig,
                    std::function<StatsSnapshot()> onGetStats);
    ~DashboardServer();

    void start(int port = 8080);
    void stop();
};
