project('wl-bubbles', 'c')

deps = []
deps += dependency('wayland-client')
deps += dependency('SDL3')
deps += dependency('SDL3_image')

wl_mod = import('wayland')

xdg_shell   = wl_mod.scan_xml(wl_mod.find_protocol('xdg-shell'))
layer_shell = wl_mod.scan_xml('/usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml')

executable('wl-bubbles', 'main.c', xdg_shell, layer_shell, dependencies : deps)
