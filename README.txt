Windows 7 bubbles -like screensaver for wayland

highly inspired by https://github.com/khang-nd/bubbles

Getting Started:
doas pacman -S sdl3 sdl3_image wayland wayland-protocols wlr-protocols
meson setup -C builddir
meson compile -C builddir
./builddir/wl-bubbles
