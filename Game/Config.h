#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }

    void reload()
    {
        // Открывает файл настроек и загружает его содержимое в объект JSON.
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        // Перегруженный оператор () позволяет удобно извлекать значения
        // настроек из загруженного JSON, используя два ключа.
        return config[setting_dir][setting_name];
    }

  private:
    json config;
};
