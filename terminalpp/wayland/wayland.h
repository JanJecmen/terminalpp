#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include <wayland-client.h>

#include "wayland-xdg-decoration-client-protocol.h"
#include "wayland-xdg-shell-client-protocol.h"

#include <fontconfig/fontconfig.h>

#include "helpers/helpers.h"

namespace wayland {

using Window = size_t;

class WaylandDisplay {
  public:
    static const WaylandDisplay& Instance() {
        static WaylandDisplay singleton;
        return singleton;
    }

    struct Interfaces {
        struct wl_compositor* compositor;
        struct xdg_wm_base* wm_base;
        struct wl_seat* seat;
        struct wl_shm* shm;
        struct zxdg_decoration_manager_v1* decoration_manager;
    };

    struct wl_display* display() const { return display_; }
    const Interfaces& interfaces() const { return interfaces_; }

    void roundtrip() const { wl_display_roundtrip(display_); }

  private:
    WaylandDisplay();
    ~WaylandDisplay();

    struct wl_display* display_;
    struct wl_registry* registry_;
    Interfaces interfaces_;
};

} // namespace wayland

#endif
