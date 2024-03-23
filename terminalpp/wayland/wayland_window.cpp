#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include "wayland_window.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

namespace tpp {

namespace {

struct wl_buffer* buffer_{nullptr};
struct zxdg_toplevel_decoration_v1* toplevel_decoration_{nullptr};
enum zxdg_toplevel_decoration_v1_mode client_preferred_mode_, current_mode_;

uint8_t* shm_data{nullptr};
size_t width_ = 600;
size_t height_ = 400;
uint8_t alpha_ = 0xdd;

/**
 * Boilerplate to create an in-memory shared file.
 *
 * Link with `-lrt`.
 */

static void randname(char* buf) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A' + (r & 15) + (r & 16) * 2;
        r >>= 5;
    }
}

static int anonymous_shm_open(void) {
    char name[] = "/hello-wayland-XXXXXX";
    int retries = 100;

    do {
        randname(name + strlen(name) - 6);

        --retries;
        // shm_open guarantees that O_CLOEXEC is set
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);

    return -1;
}

int create_shm_file(off_t size) {
    int fd = anonymous_shm_open();
    if (fd < 0) {
        return fd;
    }

    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

const char* get_mode_name(enum zxdg_toplevel_decoration_v1_mode mode) {
    switch (mode) {
    case ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE:
        return "client-side decorations";
    case ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE:
        return "server-side decorations";
    default:
        UNREACHABLE;
    }
}

void request_preferred_mode(void) {
    enum zxdg_toplevel_decoration_v1_mode mode = client_preferred_mode_;
    if (mode == 0) {
        std::cout << "Requesting compositor preferred mode\n";
        zxdg_toplevel_decoration_v1_unset_mode(toplevel_decoration_);
        return;
    }
    if (mode == current_mode_) {
        return;
    }
    std::cout << "Requesting " << get_mode_name(mode) << "\n";
    zxdg_toplevel_decoration_v1_set_mode(toplevel_decoration_, mode);
}

void get_color(size_t row, size_t col, uint8_t* r, uint8_t* g, uint8_t* b,
               uint8_t* a) {
    if (shm_data) {
        uint32_t pixel;
        memcpy(&pixel, &(((uint32_t*)shm_data)[width_ * row + col]),
               sizeof(pixel));
        *a = (pixel >> 24) & 0xff;
        *r = (pixel >> 16) & 0xff;
        *g = (pixel >> 8) & 0xff;
        *b = (pixel >> 0) & 0xff;
    }
}

void set_color(size_t row, size_t col, uint8_t r, uint8_t g, uint8_t b,
               uint8_t a) {
    if (shm_data) {
        uint32_t pixel = (a << 24) | (r << 16) | (g << 8) | (b << 0);
        memcpy(&(((uint32_t*)shm_data)[width_ * row + col]), &pixel,
               sizeof(pixel));
    }
}

void draw() {
    // UV mango with blue tint and transparency
    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            uint8_t r = (float)x / (width_ - 1) * 255.f;
            uint8_t g = (1 - ((float)y / (height_ - 1))) * 255.f;
            uint8_t b = 0xff / 2;
            uint8_t a = alpha_;
            set_color(y, x, r, g, b, a);
        }
    }

    if (current_mode_ == ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE) {
        // TODO
    }

    // Corner marks
    size_t ds = 5;
    for (size_t dy = 0; dy < ds; ++dy) {
        for (size_t dx = 0; dx < ds; ++dx) {
            uint8_t r, g, b, a;
            get_color(dy, dx, &r, &g, &b, &a);
            set_color(dy, dx, 0xff - r, 0xff - g, 0xff - b, 0xff);
            get_color(dy, width_ - 1 - dx, &r, &g, &b, &a);
            set_color(dy, width_ - 1 - dx, 0xff - r, 0xff - g, 0xff - b, 0xff);
            get_color(height_ - 1 - dy, dx, &r, &g, &b, &a);
            set_color(height_ - 1 - dy, dx, 0xff - r, 0xff - g, 0xff - b, 0xff);
            get_color(height_ - 1 - dy, width_ - 1 - dx, &r, &g, &b, &a);
            set_color(height_ - 1 - dy, width_ - 1 - dx, 0xff - r, 0xff - g,
                      0xff - b, 0xff);
        }
    }

    wl_surface_attach(surface_, buffer_, 0, 0);
    wl_surface_damage_buffer(surface_, 0, 0, width_, height_);
    wl_surface_commit(surface_);
}

void create_buffer() {
    size_t stride = width_ * 4;
    size_t size = stride * height_;
    int fd = create_shm_file(size);
    if (fd < 0) {
        fprintf(stderr, "ERROR: creating a buffer file for %luB failed: %m\n",
                size);
        return;
    }

    shm_data =
        (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED) {
        fprintf(stderr, "ERROR: mmap failed: %m\n");
        close(fd);
        return;
    }

    struct wl_shm_pool* pool = wl_shm_create_pool(shm_, fd, size);
    buffer_ = wl_shm_pool_create_buffer(pool, 0, width_, height_, stride,
                                        WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
}

const struct wl_callback_listener callback_listener = {
    .done =
        [](void*, struct wl_callback* wl_callback, uint32_t) {
            wl_callback_destroy(wl_callback);
            wl_callback = wl_surface_frame(surface_);
            wl_callback_add_listener(wl_callback, &callback_listener, nullptr);

            draw();
        },
};

const struct zxdg_toplevel_decoration_v1_listener
    toplevel_decoration_v1_listener = {
        .configure =
            [](void*, struct zxdg_toplevel_decoration_v1*, uint32_t mode) {
                enum zxdg_toplevel_decoration_v1_mode m =
                    static_cast<enum zxdg_toplevel_decoration_v1_mode>(mode);
                std::cout << "Using " << get_mode_name(m) << "\n";
                current_mode_ = m;
            },
};

} // anonymous namespace

const struct wl_keyboard_listener keyboard_listener = {
    .keymap = [](void*, struct wl_keyboard*, uint32_t, int32_t, uint32_t) {},
    .enter = [](void*, struct wl_keyboard*, uint32_t, struct wl_surface*,
                struct wl_array*) {},
    .leave = [](void*, struct wl_keyboard*, uint32_t, struct wl_surface*) {},
    .key =
        [](void*, struct wl_keyboard*, uint32_t, uint32_t, uint32_t key,
           uint32_t state) {
            if ((key == KEY_ESC || key == KEY_Q) && state == 0) {
                // todo
            }
        },
    .modifiers = [](void*, struct wl_keyboard*, uint32_t, uint32_t, uint32_t,
                    uint32_t, uint32_t) {},
    .repeat_info = [](void*, struct wl_keyboard*, int32_t, int32_t) {},
};

const struct wl_pointer_listener pointer_listener = {
    .enter = [](void*, struct wl_pointer*, uint32_t, struct wl_surface*,
                wl_fixed_t, wl_fixed_t) {},
    .leave = [](void*, struct wl_pointer*, uint32_t, struct wl_surface*) {},
    .motion = [](void*, struct wl_pointer*, uint32_t, wl_fixed_t,
                 wl_fixed_t) {},
    .button =
        [](void*, struct wl_pointer*, uint32_t, uint32_t, uint32_t button,
           uint32_t state) {
            if (button == BTN_LEFT &&
                state == WL_POINTER_BUTTON_STATE_PRESSED) {
                // todo
            }
        },
    .axis = [](void*, struct wl_pointer*, uint32_t, uint32_t, wl_fixed_t) {},
    .frame = [](void*, struct wl_pointer*) {},
    .axis_source = [](void*, struct wl_pointer*, uint32_t) {},
    .axis_stop = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
    .axis_discrete = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_value120 = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_relative_direction = [](void*, struct wl_pointer*, uint32_t,
                                  uint32_t) {},
};

const struct wl_seat_listener seat_listener = {
    .capabilities =
        [](void* data, struct wl_seat* seat, uint32_t capabilities) {
            auto& seat_data = *static_cast<WaylandWindow::SeatData*>(data);

            if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD &&
                seat_data.keyboard == nullptr) {
                seat_data.keyboard = wl_seat_get_keyboard(seat);
                wl_keyboard_add_listener(seat_data.keyboard, &keyboard_listener,
                                         nullptr);
            }
            if (capabilities & WL_SEAT_CAPABILITY_POINTER &&
                seat_data.pointer == nullptr) {
                seat_data.pointer = wl_seat_get_pointer(seat);
                wl_pointer_add_listener(seat_data.pointer, &pointer_listener,
                                        nullptr);
            }
        },
    .name = [](void*, struct wl_seat*, const char*) {},
};

const struct xdg_surface_listener surface_listener = {
    .configure =
        [](void*, struct xdg_surface* surface, uint32_t serial) {
            xdg_surface_ack_configure(surface, serial);

            // if (buffer_ == nullptr) {
            //     create_buffer();
            // }
            // draw();
        },
};

const struct xdg_toplevel_listener toplevel_listener = {
    .configure =
        [](void*, struct xdg_toplevel*, int32_t width, int32_t height,
           struct wl_array*) {
            // std::cout << "Resizing: " << width_ << "x" << height_ << " -> "
            //           << width << "x" << height << "\n";
            if (width == 0 && height == 0) {
                return;
            }
            // if (width_ != (size_t)width || height_ != (size_t)height) {
            //     munmap(shm_data, 4 * width_ * height_);
            //     width_ = (size_t)width;
            //     height_ = (size_t)height;
            //     create_buffer();
            // }
        },
    .close =
        [](void*, struct xdg_toplevel*) {
            THROW(Exception()) << "TODO: handle close";
        },
    .configure_bounds = [](void*, struct xdg_toplevel*, int32_t, int32_t) {},
    .wm_capabilities = [](void*, struct xdg_toplevel*, struct wl_array*) {},
};

WaylandWindow::WaylandWindow(std::string const& title, int cols, int rows,
                             EventQueue& eventQueue)
    : RendererWindow{cols, rows, eventQueue},
      display_{wayland::WaylandDisplay::Instance()} {
    wl_seat_add_listener(display_.interfaces().seat, &seat_listener,
                         &seat_data_);

    // create_buffer();
    // if (buffer_ == nullptr) {
    //     THROW(Exception()) << "Couldn't create buffer";
    // }

    surface_ = wl_compositor_create_surface(display_.interfaces().compositor);
    xdg_surface_ =
        xdg_wm_base_get_xdg_surface(display_.interfaces().wm_base, surface_);
    toplevel_ = xdg_surface_get_toplevel(xdg_surface_);
    xdg_surface_add_listener(xdg_surface_, &surface_listener, nullptr);
    xdg_toplevel_add_listener(toplevel_, &toplevel_listener, nullptr);
    xdg_toplevel_set_title(toplevel_, title.c_str());

    // toplevel_decoration_ =
    // zxdg_decoration_manager_v1_get_toplevel_decoration(
    //     decoration_manager_, toplevel_);

    // zxdg_toplevel_decoration_v1_add_listener(
    //     toplevel_decoration_, &toplevel_decoration_v1_listener_, nullptr);

    display_.roundtrip();

    // request_preferred_mode();

    wl_surface_commit(surface_);
    display_.roundtrip();

    // struct wl_callback* callback = wl_surface_frame(surface_);
    // wl_callback_add_listener(callback, &callback_listener_, nullptr);
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
