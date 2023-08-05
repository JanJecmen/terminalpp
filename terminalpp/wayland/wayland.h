#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include "wayland-xdg-shell-client-protocol.h"
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include <fontconfig/fontconfig.h>

namespace wayland {

typedef size_t Window;

} // namespace wayland

#endif
