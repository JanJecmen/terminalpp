#include <cstdint>
#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include <linux/input-event-codes.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "helpers/filesystem.h"
#include "helpers/time.h"

#include "wayland_window.h"

#include "wayland_application.h"

namespace tpp {

namespace {

// global client state

struct wl_compositor* compositor_{nullptr};
struct wl_shm* shm_{nullptr};
struct xdg_wm_base* wm_base_{nullptr};
struct wl_seat* seat_{nullptr};

struct wl_surface* surface_{nullptr};
struct xdg_toplevel* toplevel_{nullptr};
struct wl_keyboard* keyboard_{nullptr};
struct wl_pointer* pointer_{nullptr};
struct wl_buffer* buffer_{nullptr};

uint8_t* shm_data{nullptr};
size_t width_ = 200;
size_t height_ = 100;
uint8_t alpha_ = 0xaa;

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

    // TODO: should unmap??
    // if (shm_data) {
    //     munmap(shm_data, size);
    // }
    shm_data =
        (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED) {
        fprintf(stderr, "ERROR: mmap failed: %m\n");
        close(fd);
        return;
    }

    struct wl_shm_pool* pool = wl_shm_create_pool(shm_, fd, size);
    if (buffer_) {
        wl_buffer_destroy(buffer_);
    }
    buffer_ = wl_shm_pool_create_buffer(pool, 0, width_, height_, stride,
                                        WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
}

// listener function declarations

void callback_listener_done(void* data, struct wl_callback* wl_callback,
                            uint32_t callback_data);

void wm_base_listener_ping(void* data, struct xdg_wm_base* xdg_wm_base,
                           uint32_t serial);

void surface_listener_configure(void* data, struct xdg_surface* xdg_surface,
                                uint32_t serial);

void toplevel_listener_configure(void* data, struct xdg_toplevel* xdg_toplevel,
                                 int32_t width, int32_t height,
                                 struct wl_array* states);
void toplevel_listener_close(void* data, struct xdg_toplevel* xdg_toplevel);
void toplevel_listener_configure_bounds(void* data,
                                        struct xdg_toplevel* xdg_toplevel,
                                        int32_t width, int32_t height);
void toplevel_listener_wm_capabilities(void* data,
                                       struct xdg_toplevel* xdg_toplevel,
                                       struct wl_array* capabilities);

void keyboard_listener_keymap(void* data, struct wl_keyboard* wl_keyboard,
                              uint32_t format, int32_t fd, uint32_t size);
void keyboard_listener_enter(void* data, struct wl_keyboard* wl_keyboard,
                             uint32_t serial, struct wl_surface* surface,
                             struct wl_array* keys);
void keyboard_listener_leave(void* data, struct wl_keyboard* wl_keyboard,
                             uint32_t serial, struct wl_surface* surface);
void keyboard_listener_key(void* data, struct wl_keyboard* wl_keyboard,
                           uint32_t serial, uint32_t time, uint32_t key,
                           uint32_t state);
void keyboard_listener_modifiers(void* data, struct wl_keyboard* wl_keyboard,
                                 uint32_t serial, uint32_t mods_depressed,
                                 uint32_t mods_latched, uint32_t mods_locked,
                                 uint32_t group);
void keyboard_listener_repeat_info(void* data, struct wl_keyboard* wl_keyboard,
                                   int32_t rate, int32_t delay);

void pointer_listener_enter(void* data, struct wl_pointer* wl_pointer,
                            uint32_t serial, struct wl_surface* surface,
                            wl_fixed_t surface_x, wl_fixed_t surface_y);
void pointer_listener_leave(void* data, struct wl_pointer* wl_pointer,
                            uint32_t serial, struct wl_surface* surface);
void pointer_listener_motion(void* data, struct wl_pointer* wl_pointer,
                             uint32_t time, wl_fixed_t surface_x,
                             wl_fixed_t surface_y);
void pointer_listener_button(void* data, struct wl_pointer* wl_pointer,
                             uint32_t serial, uint32_t time, uint32_t button,
                             uint32_t state);
void pointer_listener_axis(void* data, struct wl_pointer* wl_pointer,
                           uint32_t time, uint32_t axis, wl_fixed_t value);
void pointer_listener_frame(void* data, struct wl_pointer* wl_pointer);
void pointer_listener_axis_source(void* data, struct wl_pointer* wl_pointer,
                                  uint32_t axis_source);
void pointer_listener_axis_stop(void* data, struct wl_pointer* wl_pointer,
                                uint32_t time, uint32_t axis);
void pointer_listener_axis_discrete(void* data, struct wl_pointer* wl_pointer,
                                    uint32_t axis, int32_t discrete);
void pointer_listener_axis_value120(void* data, struct wl_pointer* wl_pointer,
                                    uint32_t axis, int32_t value120);
void pointer_listener_axis_relative_direction(void* data,
                                              struct wl_pointer* wl_pointer,
                                              uint32_t axis,
                                              uint32_t direction);

void seat_listener_capabilities(void* data, struct wl_seat* wl_seat,
                                uint32_t capabilities);
void seat_listener_name(void* data, struct wl_seat* wl_seat, const char* name);

void registry_listener_global(void* data, struct wl_registry* wl_registry,
                              uint32_t name, const char* interface,
                              uint32_t version);
void registry_listener_global_remove(void* data,
                                     struct wl_registry* wl_registry,
                                     uint32_t name);

// listeners

const struct wl_callback_listener callback_listener_ = {
    .done = callback_listener_done,
};

const struct xdg_wm_base_listener wm_base_listener_ = {
    .ping = wm_base_listener_ping,
};

const struct xdg_surface_listener surface_listener_ = {
    .configure = surface_listener_configure,
};

const struct xdg_toplevel_listener toplevel_listener_ = {
    .configure = toplevel_listener_configure,
    .close = toplevel_listener_close,
    .configure_bounds = toplevel_listener_configure_bounds,
    .wm_capabilities = toplevel_listener_wm_capabilities,
};

const struct wl_keyboard_listener keyboard_listener_ = {
    .keymap = keyboard_listener_keymap,
    .enter = keyboard_listener_enter,
    .leave = keyboard_listener_leave,
    .key = keyboard_listener_key,
    .modifiers = keyboard_listener_modifiers,
    .repeat_info = keyboard_listener_repeat_info,
};

const struct wl_pointer_listener pointer_listener_ = {
    .enter = pointer_listener_enter,
    .leave = pointer_listener_leave,
    .motion = pointer_listener_motion,
    .button = pointer_listener_button,
    .axis = pointer_listener_axis,
    .frame = pointer_listener_frame,
    .axis_source = pointer_listener_axis_source,
    .axis_stop = pointer_listener_axis_stop,
    .axis_discrete = pointer_listener_axis_discrete,
    .axis_value120 = pointer_listener_axis_value120,
    .axis_relative_direction = pointer_listener_axis_relative_direction,
};

const struct wl_seat_listener seat_listener_ = {
    .capabilities = seat_listener_capabilities,
    .name = seat_listener_name,
};

const struct wl_registry_listener registry_listener_ = {
    .global = registry_listener_global,
    .global_remove = registry_listener_global_remove,
};

// listener function definitions

void callback_listener_done(void* data, struct wl_callback* wl_callback,
                            uint32_t callback_data) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(callback_data);
    wl_callback_destroy(wl_callback);
    wl_callback = wl_surface_frame(surface_);
    wl_callback_add_listener(wl_callback, &callback_listener_, nullptr);

    draw();
}

void wm_base_listener_ping(void* data, struct xdg_wm_base* xdg_wm_base,
                           uint32_t serial) {
    MARK_AS_UNUSED(data);
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void surface_listener_configure(void* data, struct xdg_surface* xdg_surface,
                                uint32_t serial) {
    MARK_AS_UNUSED(data);
    xdg_surface_ack_configure(xdg_surface, serial);

    if (buffer_ == nullptr) {
        create_buffer();
    }
    draw();
}

void toplevel_listener_configure(void* data, struct xdg_toplevel* xdg_toplevel,
                                 int32_t width, int32_t height,
                                 struct wl_array* states) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(xdg_toplevel);
    MARK_AS_UNUSED(states);
    if (width == 0 || height == 0) {
        return;
    }
    if (width_ != (size_t)width || height_ != (size_t)height) {
        width_ = (size_t)width;
        height_ = (size_t)height;
        create_buffer();
    }
}

void toplevel_listener_close(void* data, struct xdg_toplevel* xdg_toplevel) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(xdg_toplevel);
    THROW(Exception()) << "TODO: handle close";
}

void toplevel_listener_configure_bounds(void* data,
                                        struct xdg_toplevel* xdg_toplevel,
                                        int32_t width, int32_t height) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(xdg_toplevel);
    MARK_AS_UNUSED(width);
    MARK_AS_UNUSED(height);
}

void toplevel_listener_wm_capabilities(void* data,
                                       struct xdg_toplevel* xdg_toplevel,
                                       struct wl_array* capabilities) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(xdg_toplevel);
    MARK_AS_UNUSED(capabilities);
}

void keyboard_listener_keymap(void* data, struct wl_keyboard* wl_keyboard,
                              uint32_t format, int32_t fd, uint32_t size) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_keyboard);
    MARK_AS_UNUSED(format);
    MARK_AS_UNUSED(fd);
    MARK_AS_UNUSED(size);
}

void keyboard_listener_enter(void* data, struct wl_keyboard* wl_keyboard,
                             uint32_t serial, struct wl_surface* surface,
                             struct wl_array* keys) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_keyboard);
    MARK_AS_UNUSED(serial);
    MARK_AS_UNUSED(surface);
    MARK_AS_UNUSED(keys);
}

void keyboard_listener_leave(void* data, struct wl_keyboard* wl_keyboard,
                             uint32_t serial, struct wl_surface* surface) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_keyboard);
    MARK_AS_UNUSED(serial);
    MARK_AS_UNUSED(surface);
}

void keyboard_listener_key(void* data, struct wl_keyboard* wl_keyboard,
                           uint32_t serial, uint32_t time, uint32_t key,
                           uint32_t state) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_keyboard);
    MARK_AS_UNUSED(serial);
    MARK_AS_UNUSED(time);
    if ((key == KEY_ESC || key == KEY_Q) && state == 0) {
        // TODO
    } else if (key == KEY_A && state == 1) {
        // TODO
    }
}

void keyboard_listener_modifiers(void* data, struct wl_keyboard* wl_keyboard,
                                 uint32_t serial, uint32_t mods_depressed,
                                 uint32_t mods_latched, uint32_t mods_locked,
                                 uint32_t group) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_keyboard);
    MARK_AS_UNUSED(serial);
    MARK_AS_UNUSED(mods_depressed);
    MARK_AS_UNUSED(mods_latched);
    MARK_AS_UNUSED(mods_locked);
    MARK_AS_UNUSED(group);
}

void keyboard_listener_repeat_info(void* data, struct wl_keyboard* wl_keyboard,
                                   int32_t rate, int32_t delay) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_keyboard);
    MARK_AS_UNUSED(rate);
    MARK_AS_UNUSED(delay);
}

void pointer_listener_enter(void* data, struct wl_pointer* wl_pointer,
                            uint32_t serial, struct wl_surface* surface,
                            wl_fixed_t surface_x, wl_fixed_t surface_y) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(serial);
    MARK_AS_UNUSED(surface);
    MARK_AS_UNUSED(surface_x);
    MARK_AS_UNUSED(surface_y);
}

void pointer_listener_leave(void* data, struct wl_pointer* wl_pointer,
                            uint32_t serial, struct wl_surface* surface) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(serial);
    MARK_AS_UNUSED(surface);
}

void pointer_listener_motion(void* data, struct wl_pointer* wl_pointer,
                             uint32_t time, wl_fixed_t surface_x,
                             wl_fixed_t surface_y) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(time);
    MARK_AS_UNUSED(surface_x);
    MARK_AS_UNUSED(surface_y);
}

void pointer_listener_button(void* data, struct wl_pointer* wl_pointer,
                             uint32_t serial, uint32_t time, uint32_t button,
                             uint32_t state) {
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(time);
    struct wl_seat* seat = static_cast<struct wl_seat*>(data);
    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
        xdg_toplevel_move(toplevel_, seat, serial);
    }
}

void pointer_listener_axis(void* data, struct wl_pointer* wl_pointer,
                           uint32_t time, uint32_t axis, wl_fixed_t value) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(time);
    MARK_AS_UNUSED(axis);
    MARK_AS_UNUSED(value);
}

void pointer_listener_frame(void* data, struct wl_pointer* wl_pointer) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
}

void pointer_listener_axis_source(void* data, struct wl_pointer* wl_pointer,
                                  uint32_t axis_source) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(axis_source);
}

void pointer_listener_axis_stop(void* data, struct wl_pointer* wl_pointer,
                                uint32_t time, uint32_t axis) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(time);
    MARK_AS_UNUSED(axis);
}

void pointer_listener_axis_discrete(void* data, struct wl_pointer* wl_pointer,
                                    uint32_t axis, int32_t discrete) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(axis);
    MARK_AS_UNUSED(discrete);
}

void pointer_listener_axis_value120(void* data, struct wl_pointer* wl_pointer,
                                    uint32_t axis, int32_t value120) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(axis);
    MARK_AS_UNUSED(value120);
}

void pointer_listener_axis_relative_direction(void* data,
                                              struct wl_pointer* wl_pointer,
                                              uint32_t axis,
                                              uint32_t direction) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_pointer);
    MARK_AS_UNUSED(axis);
    MARK_AS_UNUSED(direction);
}

void seat_listener_capabilities(void* data, struct wl_seat* wl_seat,
                                uint32_t capabilities) {
    MARK_AS_UNUSED(data);
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD && keyboard_ == nullptr) {
        keyboard_ = wl_seat_get_keyboard(wl_seat);
        wl_keyboard_add_listener(keyboard_, &keyboard_listener_, wl_seat);
    }
    if (capabilities & WL_SEAT_CAPABILITY_POINTER && pointer_ == nullptr) {
        pointer_ = wl_seat_get_pointer(wl_seat);
        wl_pointer_add_listener(pointer_, &pointer_listener_, wl_seat);
    }
}

void seat_listener_name(void* data, struct wl_seat* wl_seat, const char* name) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_seat);
    MARK_AS_UNUSED(name);
}

void registry_listener_global(void* data, struct wl_registry* wl_registry,
                              uint32_t name, const char* interface,
                              uint32_t version) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(version);
    if (!strcmp(interface, wl_compositor_interface.name)) {
        compositor_ = static_cast<struct wl_compositor*>(
            wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4));
    } else if (!strcmp(interface, wl_shm_interface.name)) {
        shm_ = static_cast<struct wl_shm*>(
            wl_registry_bind(wl_registry, name, &wl_shm_interface, 1));
    } else if (!strcmp(interface, xdg_wm_base_interface.name)) {
        wm_base_ = static_cast<struct xdg_wm_base*>(
            wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1));
        xdg_wm_base_add_listener(wm_base_, &wm_base_listener_, nullptr);
    } else if (!strcmp(interface, wl_seat_interface.name)) {
        seat_ = static_cast<struct wl_seat*>(
            wl_registry_bind(wl_registry, name, &wl_seat_interface, 1));
        wl_seat_add_listener(seat_, &seat_listener_, nullptr);
    }
}

void registry_listener_global_remove(void* data,
                                     struct wl_registry* wl_registry,
                                     uint32_t name) {
    MARK_AS_UNUSED(data);
    MARK_AS_UNUSED(wl_registry);
    MARK_AS_UNUSED(name);
}

} // anonymous namespace

WaylandApplication::WaylandApplication() : mainLoopRunning_{false} {

    display_ = wl_display_connect(nullptr);
    if (display_ == nullptr)
        THROW(Exception()) << "Unable to connect to display";

    registry_ = wl_display_get_registry(display_);
    if (registry_ == nullptr)
        THROW(Exception()) << "Unable to get registry";

    wl_registry_add_listener(registry_, &registry_listener_, nullptr);
    wl_display_roundtrip(display_);
    wl_display_roundtrip(display_);
    if (shm_ == nullptr || compositor_ == nullptr || wm_base_ == nullptr)
        THROW(Exception())
            << "No support for wl_shm, wl_compositor, or xdg_wm_base";

    create_buffer();
    if (buffer_ == nullptr) {
        THROW(Exception()) << "Couldn't create buffer";
    }

    surface_ = wl_compositor_create_surface(compositor_);
    struct xdg_surface* xdg_surface =
        xdg_wm_base_get_xdg_surface(wm_base_, surface_);
    toplevel_ = xdg_surface_get_toplevel(xdg_surface);

    xdg_surface_add_listener(xdg_surface, &surface_listener_, nullptr);
    xdg_toplevel_add_listener(toplevel_, &toplevel_listener_, nullptr);
    xdg_toplevel_set_title(toplevel_, "terminal++");

    wl_surface_commit(surface_);
    wl_display_roundtrip(display_);

    struct wl_callback* callback = wl_surface_frame(surface_);
    wl_callback_add_listener(callback, &callback_listener_, nullptr);
}

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
        while (wl_display_dispatch(display_)) {
        }
    } catch (TerminateException const&) {
        // don't do anything
    }
    mainLoopRunning_ = false;
}

} // namespace tpp

#endif
