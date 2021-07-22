# glpaper

A X11 wallpaper slideshow-type setter using OpenGL

## Building
```
meson build
cd build
ninja
sudo ninja install
```

## Configuration

Default config path is `$XDG_CONFIG_HOME/glpaper.conf`, if XDG_CONFIG_HOME is not set it falls back to `$HOME/.config`.
Config values are as follows:
```
duration = int (duration of transition in milliseconds);
minutes = int (duration between wallpaper changed in minutes);
transitions = [ array of strings (if empty all transitions are used) ];
directory = string (Path to directory containing wallpapers);
bg-color = [ R, G, B, A (floats 0.0 - 1.0) ];
```
These settings can be configured via command line arguments as well.

## Usage

While the program is running you can run `glpaper --next` to advance to the next wallpaper, or `glpaper --reload` to reload the configuration.
You can view a list of available transitions by using `glpaper --help` (when no glpaper instance is running).
