#pragma once

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <chrono>
#include <dbus/dbus.h>
#include <memory>
#include <vector>

using std::chrono::steady_clock;

class Config;
class Shader;
class Texture;

class PaperWindow
{
public:
    PaperWindow(DBusConnection* bus, Config* cfg);
    ~PaperWindow();

    int run();

private:
    void setup_vbo();
    void create_shader();
    void load_textures();
    void set_uniforms();

    void setup_transition();
    void start_transition();
    void load_paths();
    std::string get_random_texture_path();

    DBusConnection* m_Bus;
    int m_Width, m_Height;
    std::string m_Transition;

    std::unique_ptr<Config> m_Config;
    std::unique_ptr<Shader> m_Shader;
    std::unique_ptr<Texture> m_CurrentTexture, m_NextTexture;

    std::vector<std::string> m_WallpaperPaths;

    steady_clock::time_point m_TransitionStart, m_TransitionEnd;
    bool m_Animating{ false };

    Display* m_Display;
    Window m_Window;

    GLXFBConfig m_FBconfig;
    GLXContext m_Context;
};
