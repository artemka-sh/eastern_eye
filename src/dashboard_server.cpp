#include "dashboard_server.hpp"
#include "httplib.h"
#include <rfl/json.hpp>
#include <print>
#include <fstream>

DashboardServer::DashboardServer(SharedState& state,
                                 std::function<AppConfig()> onGetConfig,
                                 std::function<void(const AppConfig&)> onSetConfig,
                                 std::function<StatsSnapshot()> onGetStats)
    : server_(std::make_unique<httplib::Server>())
    , state_(state)
    , onGetConfig_(std::move(onGetConfig))
    , onSetConfig_(std::move(onSetConfig))
    , onGetStats_(std::move(onGetStats))
{
    server_->Get("/", [this](const httplib::Request&, httplib::Response& res)
    {
        res.set_content(indexHtml_, "text/html");
    });

    server_->Get("/api/config", [this](const httplib::Request&, httplib::Response& res)
    {
        std::lock_guard lock(state_.mtx);
        res.set_content(rfl::json::write(onGetConfig_()), "application/json");
    });

    server_->Post("/api/config", [this](const httplib::Request& req, httplib::Response& res)
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

    server_->Get("/api/stats", [this](const httplib::Request&, httplib::Response& res)
    {
        std::lock_guard lock(state_.mtx);
        res.set_content(rfl::json::write(onGetStats_()), "application/json");
    });
}

DashboardServer::~DashboardServer()
{
    stop();
}

void DashboardServer::start(int port)
{
    std::ifstream htmlStream("res/index.html");
    if (htmlStream.is_open())
    {
        indexHtml_.assign(std::istreambuf_iterator<char>(htmlStream),
                          std::istreambuf_iterator<char>());
    }
    else
    {
        indexHtml_ = "Ошибка чтения html страницы";
        std::println(stderr, "ОШИБКА ЧТЕНИЯ HTML СТРАНИЦЫ!");
    }

    serverThread_ = std::thread([this, port]()
    {
        std::println("Сервер запущен на порту {}", port);
        server_->listen("0.0.0.0", port);
    });
}

void DashboardServer::stop()
{
    server_->stop();
    if (serverThread_.joinable())
        serverThread_.join();
}
