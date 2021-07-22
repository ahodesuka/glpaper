#include "window.hh"

#include <random/random.hh>
using Random = effolkronium::random_static;

#include "config.hh"
#include "shader.hh"
#include "texture.hh"
#include "transitions.hh"

#include <GL/glext.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shapeconst.h>
#include <chrono>
#include <filesystem>
namespace fs = std::filesystem;

#include <map>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>
#include <stdexcept>
#include <stdio.h>
#include <unistd.h>

// clang-format off
static const std::string vert_shader_source =
"#version 330 core\n"
"layout (location = 0) in vec2 position;\n"
"out vec2 uv;\n"
"void main(void) {\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  uv = position * 0.5 + 0.5;\n"
"}\n";
static const char* frag_shader_template =
"#version 330 core\n"
"in vec2 uv;\n"
"out vec4 FragColor;\n"
"uniform sampler2D from;\n"
"uniform sampler2D to;\n"
"uniform float progress;\n"
"uniform float ratio;\n"
"\n"
"vec4 getFromColor(vec2 _uv) {\n"
"  return texture(from, _uv);\n"
"}\n"
"\n"
"vec4 getToColor(vec2 _uv) {\n"
"  return texture(to, _uv);\n"
"}\n"
"\n"
"\n%s\n"
"void main() {\n"
"  FragColor = transition(uv);\n"
"}\n";
// clang-format on

PaperWindow::PaperWindow(DBusConnection* bus, Config* cfg)
    : m_Bus{ bus },
      m_Config{ std::move(cfg) },
      m_TransitionStart{ steady_clock::now() }
{
    m_Display = XOpenDisplay(nullptr);
    if (!m_Display)
        throw std::runtime_error("Failed to open X11 display");

    int screen_num{ DefaultScreen(m_Display) };

    m_Width  = DisplayWidth(m_Display, screen_num);
    m_Height = DisplayHeight(m_Display, screen_num);

    // clang-format off
    int attr[] = { 
        GLX_X_RENDERABLE,    true,
        GLX_DRAWABLE_TYPE,   GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,     GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE,   GLX_TRUE_COLOR,
        GLX_DOUBLEBUFFER,    true,
        GLX_RED_SIZE,        8,
        GLX_GREEN_SIZE,      8,
        GLX_BLUE_SIZE,       8,
        GLX_DEPTH_SIZE,      0,
        GLX_STENCIL_SIZE,    0,
        0
    };
    // clang-format on

    int n_fbc;
    GLXFBConfig* fb_configs{ glXChooseFBConfig(m_Display, screen_num, attr, &n_fbc) };
    if (!fb_configs)
        throw std::runtime_error("Failed to get FB configs");

    // Get a working fbconfig
    for (int i = 0; i < n_fbc; ++i)
    {
        XVisualInfo* vi{ glXGetVisualFromFBConfig(m_Display, fb_configs[i]) };

        if (!vi)
            continue;

        XRenderPictFormat* pict_format{ XRenderFindVisualFormat(m_Display, vi->visual) };
        XFree(vi);

        if (!pict_format)
            continue;

        m_FBconfig = fb_configs[i];
        break;
    }

    XFree(fb_configs);

    if (!m_FBconfig)
        throw std::runtime_error("Failed to get visual");

    XVisualInfo* vi{ glXGetVisualFromFBConfig(m_Display, m_FBconfig) };
    Window root_window{ RootWindow(m_Display, screen_num) };
    XSetWindowAttributes swa = { 0 };
    swa.background_pixel     = 0;
    swa.border_pixel         = 0;
    swa.colormap             = XCreateColormap(m_Display, root_window, vi->visual, AllocNone);
    swa.event_mask           = ExposureMask | SubstructureNotifyMask;
    swa.override_redirect    = 1;
    unsigned long mask =
        CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;
    m_Window = XCreateWindow(m_Display,
                             root_window,
                             0,
                             0,
                             m_Width,
                             m_Height,
                             0,
                             vi->depth,
                             InputOutput,
                             vi->visual,
                             mask,
                             &swa);
    // Allow all input to passthrough this window to anything behind it
    XRectangle rect;
    XserverRegion region{ XFixesCreateRegion(m_Display, &rect, 1) };
    XFixesSetWindowShapeRegion(m_Display, m_Window, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(m_Display, region);

    // Tell the WM that this is a desktop window
    Atom value{ XInternAtom(m_Display, "_NET_WM_WINDOW_TYPE_DESKTOP", false) };
    XChangeProperty(m_Display,
                    m_Window,
                    XInternAtom(m_Display, "_NET_WM_WINDOW_TYPE", false),
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (unsigned char*)&value,
                    1);

    // clang-format off
	int gl3attr[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_FLAGS_ARB,         GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
    };
    // clang-format on

    m_Context = glXCreateContextAttribsARB(m_Display, m_FBconfig, nullptr, true, gl3attr);
    if (!m_Context)
        throw std::runtime_error("Failed to create OpenGL context");

    XMapWindow(m_Display, m_Window);
    // Lower the window so it's behind every other window
    XLowerWindow(m_Display, m_Window);

    glXMakeCurrent(m_Display, m_Window, m_Context);
    // Enable adaptive vsync
    glXSwapIntervalEXT(m_Display, m_Window, -1);
}

PaperWindow::~PaperWindow()
{
    // cleanup stuff enough though it doesnt matter
}

int PaperWindow::run()
{
    load_paths();
    setup_transition();
    setup_vbo();

    bool first_draw{ true };

    while (true)
    {
        auto now{ steady_clock::now() };
        std::apply(glClearColor, m_Config->get_bg_color());
        glClear(GL_COLOR_BUFFER_BIT);

        if (m_Animating)
        {
            float dur{ std::chrono::duration<float>(m_TransitionEnd - m_TransitionStart).count() };
            float t{ std::chrono::duration<float>(steady_clock::now() - m_TransitionStart).count() /
                     dur };
            t = std::min(1.0f, t);
            m_Shader->set_1f("progress", t);

            if (t >= 1.0f)
            {
                m_Animating       = false;
                m_TransitionStart = steady_clock::now();
                // setup for the next transition
                setup_transition();
            }
        }
        else if (!first_draw)
        {
            while (true)
            {
                using namespace std::chrono;
                dbus_connection_read_write(m_Bus, 0);
                DBusMessage* msg{ dbus_connection_pop_message(m_Bus) };

                if (msg)
                {
                    spdlog::info("msg");
                    if (dbus_message_is_method_call(
                            msg, "com.github.ahodesuka.glpaper.new", "new_wallpaper"))
                    {
                        break;
                    }
                    else if (dbus_message_is_method_call(
                                 msg, "com.github.ahodesuka.glpaper.reload", "reload_config"))
                    {
                        // FIXME: This should do things when things change
                        m_Config->load_config(true);
                        load_paths();
                        setup_transition();
                        break;
                    }

                    dbus_message_unref(msg);
                }
                else
                {
                    // 100ms
                    usleep(100000);
                }

                if (duration_cast<milliseconds>(steady_clock::now() - m_TransitionStart) >=
                    m_Config->get_display_duration())
                    break;
            }

            // continue here because we need to set the updated time delta in the shader before
            // rendering
            start_transition();
            continue;
        }
        else
        {
            first_draw = false;
        }

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glXSwapBuffers(m_Display, m_Window);
    }

    return EXIT_SUCCESS;
}

void PaperWindow::setup_vbo()
{
    // clang-format off
    static const GLfloat vertices[] = {
        -1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
    };
    // clang-format on

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLint loc{ glGetAttribLocation(m_Shader->get_id(), "position") };
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
}

void PaperWindow::create_shader()
{
    std::string transition_str;
    try
    {
        transition_str = reinterpret_cast<const char*>(transitions_map.at(m_Transition));
    }
    catch (const std::out_of_range&)
    {
        if (m_Transition != "fade")
        {
            spdlog::error(
                fmt::format("Failed to find transition '{}', falling back to fade", m_Transition));
            m_Transition   = "fade";
            transition_str = reinterpret_cast<const char*>(transitions_map.at(m_Transition));
        }
        else
        {
            throw std::runtime_error("Failed to find fade transition");
        }
    }

    auto len{ strlen(frag_shader_template) + transition_str.length() };
    std::string frag(len, ' ');
    snprintf(&frag[0], len, frag_shader_template, transition_str.c_str());

    m_Shader = std::make_unique<Shader>(vert_shader_source, frag);

    if (m_Shader->get_id() == 0 && m_Transition != "fade")
    {
        spdlog::error(
            fmt::format("Failed to compile transition '{}', falling back to fade", m_Transition));
        m_Transition = "fade";
        create_shader();
    }
}

void PaperWindow::load_textures()
{
    if (m_CurrentTexture)
    {
        m_CurrentTexture = std::move(m_NextTexture);
        m_NextTexture    = std::make_unique<Texture>(get_random_texture_path());
    }
    else
    {
        m_CurrentTexture = std::make_unique<Texture>(get_random_texture_path());
        m_NextTexture    = std::make_unique<Texture>(get_random_texture_path());
    }

    m_CurrentTexture->bind(0);
    m_Shader->set_1i("from", 0);
    m_NextTexture->bind(1);
    m_Shader->set_1i("to", 1);
}

void PaperWindow::set_uniforms()
{
    auto ratio{ m_Width / m_Height };

    if (m_Transition == "circlecrop" || m_Transition == "gridflip")
    {
        m_Shader->set_4f("bgcolor", m_Config->get_bg_color());
    }

    if (m_Transition == "gridflip")
    {
        m_Shader->set_2f("size", { ratio * 4.0f, 4.0f });
        m_Shader->set_1f("dividerWidth", 0.05f / ratio);
    }

    if (m_Transition == "randomsquares" || m_Transition == "squareswire")
    {
        m_Shader->set_2f("size", { ratio * 10.0f, 10.0f });
    }

    if (m_Transition == "squareswire" || m_Transition == "directionalwarp" ||
        m_Transition == "directionalwipe")
    {
        m_Shader->set_2f("direction", { Random::get(-1.0f, 1.0f), Random::get(-1.0f, 1.0f) });
    }

    if (m_Transition == "directional_easing" || m_Transition == "directional")
    {
        auto x{ Random::get(0.0f, 1.0f) };
        auto y = x == 1.0f ? 0.0f : 1.0f;
        m_Shader->set_2f("direction", { x, y });
    }

    if (m_Transition == "luminance_melt")
    {
        m_Shader->set_1i("direction", Random::get(0, 1));
    }
}

void PaperWindow::setup_transition()
{
    if (m_Config->get_enabled_transitions().empty())
        m_Transition = Random::get(transitions_map)->first;
    else
        m_Transition = *Random::get(m_Config->get_enabled_transitions());

    create_shader();

    glUseProgram(m_Shader->get_id());
    m_Shader->set_1f("ratio", static_cast<float>(m_Width) / m_Height);
    m_Shader->set_1f("progress", 0.0f);

    set_uniforms();
    load_textures();
}

void PaperWindow::start_transition()
{
    m_Animating       = true;
    m_TransitionStart = steady_clock::now();
    m_TransitionEnd   = m_TransitionStart + m_Config->get_transition_duration();
}

void PaperWindow::load_paths()
{
    m_WallpaperPaths.clear();

    for (const auto& entry : fs::directory_iterator(m_Config->get_wallpaper_directory()))
        if (stbi_info(entry.path().c_str(), nullptr, nullptr, nullptr))
            m_WallpaperPaths.emplace_back(entry.path());

    if (m_WallpaperPaths.size() < 2)
        throw std::runtime_error("Wallpaper directory contains less than 2 valid image files");
}

std::string PaperWindow::get_random_texture_path()
{
    auto iter{ Random::get(m_WallpaperPaths) };

    if (m_CurrentTexture && m_CurrentTexture->get_path() == *iter)
    {
        // Wrap the iter if its the last one
        if (++iter == m_WallpaperPaths.end())
            iter = m_WallpaperPaths.begin();
    }

    return *iter;
}
