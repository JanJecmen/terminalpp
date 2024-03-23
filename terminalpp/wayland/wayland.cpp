#include <wayland-client-protocol.h>
#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include "wayland.h"

#include <cstring>

namespace wayland {

const struct xdg_wm_base_listener wm_base_listener = {
    .ping = [](void*, struct xdg_wm_base* wm_base,
               uint32_t serial) { xdg_wm_base_pong(wm_base, serial); },
};

const struct wl_registry_listener registry_listener = {
    .global =
        [](void* data, struct wl_registry* registry, uint32_t name,
           const char* interface, uint32_t) {
            auto& interfaces = *static_cast<WaylandDisplay::Interfaces*>(data);

            if (!std::strcmp(interface, wl_compositor_interface.name)) {
                interfaces.compositor =
                    static_cast<struct wl_compositor*>(wl_registry_bind(
                        registry, name, &wl_compositor_interface, 4));
            }

            if (!std::strcmp(interface, wl_shm_interface.name)) {
                interfaces.shm = static_cast<struct wl_shm*>(
                    wl_registry_bind(registry, name, &wl_shm_interface, 1));
            }

            if (!std::strcmp(interface, xdg_wm_base_interface.name)) {
                interfaces.wm_base =
                    static_cast<struct xdg_wm_base*>(wl_registry_bind(
                        registry, name, &xdg_wm_base_interface, 1));
            }

            if (!std::strcmp(interface,
                             zxdg_decoration_manager_v1_interface.name)) {
                interfaces.decoration_manager =
                    static_cast<struct zxdg_decoration_manager_v1*>(
                        wl_registry_bind(registry, name,
                                         &zxdg_decoration_manager_v1_interface,
                                         1));
            }

            if (!std::strcmp(interface, wl_seat_interface.name)) {
                interfaces.seat = static_cast<struct wl_seat*>(
                    wl_registry_bind(registry, name, &wl_seat_interface, 1));
            }
        },
    .global_remove = [](void*, struct wl_registry*, uint32_t) {},
};

WaylandDisplay::WaylandDisplay() {
    display_ = wl_display_connect(nullptr);
    if (display_ == nullptr)
        THROW(Exception()) << "Unable to connect to display";

    registry_ = wl_display_get_registry(display_);
    if (registry_ == nullptr)
        THROW(Exception()) << "Unable to get registry";

    wl_registry_add_listener(registry_, &registry_listener, &interfaces_);
    wl_display_roundtrip(display_);
    wl_display_roundtrip(display_);
    if (interfaces_.compositor == nullptr)
        THROW(Exception()) << "No support for wl_compositor";
    if (interfaces_.wm_base)
        THROW(Exception()) << "No support for xdg_wm_base";
    if (interfaces_.shm == nullptr)
        THROW(Exception()) << "No support for wl_shm";

    xdg_wm_base_add_listener(interfaces_.wm_base, &wm_base_listener, nullptr);
}

WaylandDisplay::~WaylandDisplay() {
    if (interfaces_.seat)
        wl_seat_destroy(interfaces_.seat);
    if (interfaces_.wm_base)
        xdg_wm_base_destroy(interfaces_.wm_base);
    if (interfaces_.compositor)
        wl_compositor_destroy(interfaces_.compositor);
    if (interfaces_.decoration_manager)
        zxdg_decoration_manager_v1_destroy(interfaces_.decoration_manager);
    if (interfaces_.shm)
        wl_shm_destroy(interfaces_.shm);
    if (registry_)
        wl_registry_destroy(registry_);
    if (display_)
        wl_display_disconnect(display_);
}

} // namespace wayland

#endif
