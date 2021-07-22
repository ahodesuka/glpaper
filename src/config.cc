#include "config.hh"

#include <cxxopts/cxxopts.hh>
#include <filesystem>
namespace fs = std::filesystem;

#include <libconfig.h++>
#include <spdlog/spdlog.h>
#include <stdlib.h>

Config::Config(cxxopts::ParseResult& res)
    : m_BGColor{ 0.08f, 0.08f, 0.08f, 1.0f },
      m_TransitionDuration{ 3000 },
      m_DisplayDuration{ std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::minutes(120)) }
{
    if (res.count("config"))
    {
        auto path{ res["config"].as<std::string>() };

        if (fs::exists(path))
            m_ConfigPath = std::move(path);
    }

    if (m_ConfigPath.empty())
    {
        const char* xdg_config_home{ getenv("XDG_CONFIG_HOME") };
        if (xdg_config_home)
        {
            m_ConfigPath = std::string(xdg_config_home) + "/glpaper.conf";
        }
        else
        {
            const char* home{ getenv("HOME") };
            if (home)
                m_ConfigPath = std::string(home) + "/.config/glpaper.conf";
        }
    }

    if (res.count("directory"))
    {
        auto path{ res["directory"].as<std::string>() };

        if (fs::exists(path))
            m_DirectoryPath = std::move(path);
    }

    if (res.count("bg-color"))
    {
        auto bg{ res["bg-color"].as<std::vector<float>>() };

        if (bg.size() == 4)
        {
            std::copy_n(bg.begin(), 4, m_BGColor.begin());
            m_BGColorSet = true;
        }
        else
        {
            spdlog::error(
                "Invalid bg-color given, must be in RGBA format ie. --bg-color=0.0,0.0,0.0,1.0");
        }
    }

    if (res.count("transitions"))
        m_EnabledTransitions = res["transitions"].as<std::vector<std::string>>();

    if (res.count("duration"))
    {
        m_TransitionDuration    = std::chrono::milliseconds{ res["duration"].as<int>() };
        m_TransitionDurationSet = true;
    }

    if (res.count("minutes"))
    {
        m_DisplayDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::minutes(res["transitions"].as<int>()));
        m_DisplayDurationSet = true;
    }

    load_config();
}

void Config::load_config(bool reload)
{
    libconfig::Config config;
    config.readFile(m_ConfigPath.c_str());

    if ((reload || m_DirectoryPath.empty()) && config.exists("directory"))
    {
        std::string tmp;
        config.lookupValue("directory", tmp);

        if (fs::exists(tmp))
            m_DirectoryPath = std::move(tmp);
    }

    if ((reload || !m_BGColorSet) && config.exists("bg-color"))
    {
        auto& bg_color{ config.lookup("bg-color") };
        std::array<float, 4> tmp;
        int n{ -1 };

        for (auto& v : bg_color)
        {
            if (++n >= 4)
            {
                spdlog::error("Invalid bg-color in config, must be in RGBA format ie. bg-color = [ "
                              "0.0, 0.0, 0.0, 1.0] ");
                break;
            }

            tmp[n] = v;
        }

        if (n == 3)
            m_BGColor = std::move(tmp);
    }

    if ((reload || m_EnabledTransitions.empty()) && config.exists("transitions"))
    {
        m_EnabledTransitions.clear();

        auto& transitions{ config.lookup("transitions") };
        for (auto& v : transitions)
            m_EnabledTransitions.emplace_back(v);
    }

    if ((reload || !m_TransitionDurationSet) && config.exists("duration"))
    {
        int dur;
        config.lookupValue("duration", dur);
        m_TransitionDuration = std::chrono::milliseconds(dur);
    }

    if ((reload || !m_DisplayDurationSet) && config.exists("minutes"))
    {
        int dur;
        config.lookupValue("minutes", dur);
        m_DisplayDuration =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(dur));
    }

    if (m_DirectoryPath.empty())
        throw std::runtime_error("Wallpaper directory was not provided");
}
