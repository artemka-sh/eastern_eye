#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include "system_configuration.hpp"
#include "httplib.h"
#include <rfl/json.hpp>
#include <print>

struct SharedState {
    std::mutex mtx;
    std::vector<uchar> latestJpegBuffer;
};

class DashboardServer
{
    httplib::Server server_;
    std::thread serverThread_;
    SharedState& state_;
    std::function<AppConfig()> onGetConfig_;
    std::function<void(const AppConfig&)> onSetConfig_;

public:
    DashboardServer(SharedState& state,
                    std::function<AppConfig()> onGetConfig,
                    std::function<void(const AppConfig&)> onSetConfig)
        : state_(state)
        , onGetConfig_(std::move(onGetConfig))
        , onSetConfig_(std::move(onSetConfig))
    {
        server_.Get("/api/config", [this](const httplib::Request&, httplib::Response& res)
        {
            std::lock_guard lock(state_.mtx);
            res.set_content(rfl::json::write(onGetConfig_()), "application/json");
        });

        server_.Post("/api/config", [this](const httplib::Request& req, httplib::Response& res)
        {
            auto result = rfl::json::read<AppConfig>(req.body);
            if (!result)
            {
                res.status = 400;
                res.set_content("{\"error\":\"некорректный json\"}", "application/json");
                return;
            }
            std::lock_guard lock(state_.mtx);
            onSetConfig_(result.value());
            res.set_content("{\"status\":\"ok\"}", "application/json");
        });

        server_.Get("/api/stats", [this](const httplib::Request&, httplib::Response& res)
        {
            res.set_content("{\"status\":\"running\"}", "application/json");
        });
    }

    void start(int port = 8080)
    {
        serverThread_ = std::thread([this, port]()
        {
            std::println("Сервер запущен на порту {}", port);
            server_.listen("0.0.0.0", port);
        });
    }

    void stop()
    {
        server_.stop();
        if (serverThread_.joinable())
            serverThread_.join();
    }
};
