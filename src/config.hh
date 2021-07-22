#pragma once

#include <array>
#include <chrono>
#include <string>
#include <vector>

namespace cxxopts
{
    class ParseResult;
}

class Config
{
public:
    Config(cxxopts::ParseResult& res);
    ~Config() = default;

    // If reload is true existing values are overwritten
    void load_config(bool reload = false);

    std::string get_wallpaper_directory() const { return m_DirectoryPath; }

    const std::array<float, 4>& get_bg_color() const { return m_BGColor; }
    const std::vector<std::string>& get_enabled_transitions() const { return m_EnabledTransitions; }

    std::chrono::milliseconds get_transition_duration() const { return m_TransitionDuration; }
    std::chrono::milliseconds get_display_duration() const { return m_DisplayDuration; }

private:
    bool m_BGColorSet{ false }, m_TransitionDurationSet{ false }, m_DisplayDurationSet{ false };
    std::string m_DirectoryPath, m_ConfigPath;
    std::array<float, 4> m_BGColor;
    std::vector<std::string> m_EnabledTransitions;
    std::chrono::milliseconds m_TransitionDuration, m_DisplayDuration;
};
