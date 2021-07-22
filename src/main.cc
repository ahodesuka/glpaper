#include "config.hh"
#include "transitions.hh"
#include "window.hh"

#include <cxxopts/cxxopts.hh>
#include <dbus/dbus.h>
#include <spdlog/spdlog.h>

DBusConnection* bus{ nullptr };

// Creates a dbus connection and returns whether this is the primary instance
bool create_dbus_connection(bool caller = false)
{
    std::string name{ caller ? "com.github.ahodesuka.glpaper.caller"
                             : "com.github.ahodesuka.glpaper.primary" };

    int ret_code{ 0 };
    DBusError err{ 0 };

    bus = dbus_bus_get(DBUS_BUS_SESSION, &err);

    if (!bus)
        goto error;

    ret_code = dbus_bus_request_name(bus,
                                     name.c_str(),
                                     caller ? DBUS_NAME_FLAG_REPLACE_EXISTING
                                            : DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                     &err);

    if (ret_code < 0)
        goto error;

    if (ret_code == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
        return !caller;

error:
    if (dbus_error_is_set(&err))
    {
        spdlog::error(fmt::format("D-Bus error: {}", err.message));
        dbus_error_free(&err);

        return false;
    }

    return caller ? false : create_dbus_connection(true);
}

void check_print_help(cxxopts::Options& opts) { }

int main(int argc, char** argv)
{
    bool is_primary{ create_dbus_connection() };
    cxxopts::Options opts{ "glpaper", "X11 wallpaper setter using OpenGL" };

    opts.set_width(80);
    opts.set_tab_expansion();

    if (is_primary)
    {
        std::string transition_list{ "\n" };
        for (auto [name, src] : transitions_map)
            transition_list.append("\t").append(name).append("\n");

        // clang-format off
        opts.add_options("Settings")
            ("b,bg-color", "RGBA values for the background color (0.0-1.0 ranges)", cxxopts::value<std::vector<float>>())
            ("c,config", "Path to the config file ($XDG_CONFIG_HOME/glpaper.conf is the default)", cxxopts::value<std::string>())
            ("d,duration", "Transition duration in milliseconds", cxxopts::value<int>())
            ("m,minutes", "Number of minutes between wallpaper changes", cxxopts::value<int>())
            ("t,transitions", "A list of transition names, available transitions:" + transition_list, cxxopts::value<std::vector<std::string>>())
            ("w,directory", "Wallpaper directory containing image files", cxxopts::value<std::string>())
        ;
        // clang-format on
    }
    else
    {
        // clang-format off
        opts.add_options("Action")
            ("n,next", "Next wallpaper")
            ("r,reload", "Reload configuration")
        ;
        // clang-format on
    }

    // clang-format off
    opts.add_options("Usage")
        ("h,help", "Print this help message")
    ;
    // clang-format on

    auto result{ opts.parse(argc, argv) };
    if (result.count("help"))
    {
        std::cout << opts.help() << std::endl;
        return EXIT_SUCCESS;
    }

    if (!is_primary)
    {
        if (result.count("reload"))
        {
            DBusMessage* msg{ dbus_message_new_method_call("com.github.ahodesuka.glpaper.primary",
                                                           "/com/github/ahodesuka/glpaper/reload",
                                                           "com.github.ahodesuka.glpaper.reload",
                                                           "reload_config") };
            dbus_connection_send(bus, msg, nullptr);
            dbus_connection_flush(bus);
            dbus_message_unref(msg);
        }

        if (result.count("next"))
        {
            DBusMessage* msg{ dbus_message_new_method_call("com.github.ahodesuka.glpaper.primary",
                                                           "/com/github/ahodesuka/glpaper/new",
                                                           "com.github.ahodesuka.glpaper.new",
                                                           "new_wallpaper") };
            dbus_connection_send(bus, msg, nullptr);
            dbus_connection_flush(bus);
            dbus_message_unref(msg);
        }

        return EXIT_SUCCESS;
    }
    else
    {
        Config* config{ new Config{ result } };
        PaperWindow* window;

        try
        {
            window = new PaperWindow(bus, config);
        }
        catch (const std::runtime_error& e)
        {
            spdlog::error(e.what());
            return EXIT_FAILURE;
        }

        return window->run();
    }
}
