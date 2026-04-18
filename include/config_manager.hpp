//
// Created by artem on 17.04.2026.
//

#pragma once
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <print>
#include <rfl/json.hpp>

class ConfigManager
{
    // std::map<std::string, std::string> configs;
public:
    template <typename T>
    static std::string getTypeName()
    {
        std::string_view name = __PRETTY_FUNCTION__;
        // пример того как выглядит:
        // "static std::string ConfigManager::getTypeName() [with T = BoardTrackerConfig]"

        size_t start = name.find("T = ") + 4;
        size_t end = name.find_first_of("];", start);

        return std::string(name.substr(start, end - start));
    }

    template<typename T>
    static void save(const T& configPtr, std::string filename = "")
    {
        if (filename.empty()) filename = getTypeName<T>() + ".json";

        std::string json_str = rfl::json::write(configPtr);

        std::println("голый json выглядит при сериализации так: {}", json_str);
        std::ofstream jsonfile_s(filename);
        if (jsonfile_s.is_open())
        {
            jsonfile_s << json_str;
            std::println("Конфиг сохранён в {}", filename);
            jsonfile_s.close();
        }
    }


    template <typename T>
    static bool load(T& config, std::string filename = "")
    {
        if (filename.empty()) filename = getTypeName<T>() + ".json";

        std::ifstream jsonfile_s(filename);
        if (!jsonfile_s.is_open()) {
            std::println("Конфигурация {} не найдена, используются базовые параметры", filename);
            return false;
        }

        std::string json_str((std::istreambuf_iterator<char>(jsonfile_s)),
                              std::istreambuf_iterator<char>());

        // rfl::json::read возвращает Result (может быть ошибка парсинга)
        auto result = rfl::json::read<T>(json_str);

        if (result) {
            config = result.value(); // Успех! Обновляем структуру
            return true;
        } else {
            std::println(stderr, "Парсинг рухнул на файле {} : {}", filename, result.error().what());
            return false;
        }
    }
    template <typename T>
    static bool sync(T& config, const std::string& filename = "")
    {
        load(config, filename);
        save(config, filename);
        return true;
    }
};
