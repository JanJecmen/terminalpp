# Wayland support notes and TODOs

## Roadplan

- cmake wayland detection
- cmake build and link with wayland
- run a simple window under wayland without any text rendering
- add text rendering
- window decorations
- move rendering to gpu

- add pacman (and maybe other - dnf, zypper) variants of the setup script

- don't use ecm for finding wayland in cmake - have something custom w/o dependencies

## Decide

Which text rendering library?
- freetype
- pango
- cairo
- fcft
- libschrift
- libdrawtext

How to support both wayland and x11?
- have two separate builds, only x11 and only wayland
- have one build, all linked statically, decide what to run based on an env variable at runtime
- build two shared libraries, decide at runtime which one to load

First, just disable x11 and use wayland only
Later figure out a better build setup

## Notes

## Sources

