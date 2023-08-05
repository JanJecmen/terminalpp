#include "helpers/helpers.h"
#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include "wayland_window.h"

namespace tpp {

WaylandWindow::WaylandWindow(std::string const& title, int cols, int rows,
                             EventQueue& eventQueue)
    : RendererWindow{cols, rows, eventQueue} {
    MARK_AS_UNUSED(title);
}

WaylandWindow::~WaylandWindow() {}

void WaylandWindow::setTitle(std::string const& value) {
    RendererWindow::setTitle(value);
}

void WaylandWindow::setIcon(Window::Icon icon) {
    RendererWindow::setIcon(icon);
}

void WaylandWindow::setFullscreen(bool value) { MARK_AS_UNUSED(value); }

void WaylandWindow::show(bool value) { MARK_AS_UNUSED(value); }

void WaylandWindow::schedule(std::function<void()> event, Widget* widget) {
    RendererWindow::schedule(event, widget);
}

void WaylandWindow::requestClipboard(Widget* sender) {
    RendererWindow::requestClipboard(sender);
}

void WaylandWindow::requestSelection(Widget* sender) {
    RendererWindow::requestSelection(sender);
}

void WaylandWindow::setClipboard(std::string const& contents) {
    MARK_AS_UNUSED(contents);
}

void WaylandWindow::setSelection(std::string const& contents, Widget* owner) {
    MARK_AS_UNUSED(contents);
    MARK_AS_UNUSED(owner);
}

void WaylandWindow::clearSelection(Widget* sender) {
    RendererWindow::clearSelection(sender);
}

} // namespace tpp

#endif
