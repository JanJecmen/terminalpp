#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include <wayland-client.h>

#include "wayland-xdg-decoration-client-protocol.h"
#include "wayland-xdg-shell-client-protocol.h"

#include <fontconfig/fontconfig.h>

namespace wayland {

using Window = size_t;

} // namespace wayland

#endif
