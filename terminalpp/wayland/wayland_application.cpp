#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include "helpers/filesystem.h"
#include "helpers/time.h"

#include "wayland_window.h"

#include "wayland_application.h"

namespace tpp {

WaylandApplication::WaylandApplication() : mainLoopRunning_{false} {}

WaylandApplication::~WaylandApplication() {}

void WaylandApplication::alert(std::string const& message) {
    MARK_AS_UNUSED(message);
}

bool WaylandApplication::query(std::string const& title,
                               std::string const& message) {
    MARK_AS_UNUSED(title);
    MARK_AS_UNUSED(message);
    return true;
}

void WaylandApplication::openLocalFile(std::string const& filename, bool edit) {
    MARK_AS_UNUSED(filename);
    MARK_AS_UNUSED(edit);
}

void WaylandApplication::openUrl(std::string const& url) {
    MARK_AS_UNUSED(url);
}

void WaylandApplication::setClipboard(std::string const& contents) {
    MARK_AS_UNUSED(contents);
}

Window* WaylandApplication::createWindow(std::string const& title, int cols,
                                         int rows) {
    return new WaylandWindow{title, cols, rows, eventQueue_};
}

void WaylandApplication::mainLoop() {
    mainLoopRunning_ = true;
    try {
        while (true) {
        }
    } catch (TerminateException const&) {
        // don't do anything
    }
    mainLoopRunning_ = false;
}

} // namespace tpp

#endif
