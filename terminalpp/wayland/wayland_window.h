#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include "wayland.h"

#include "../window.h"
#include "wayland_font.h"

namespace tpp {

using namespace ui;

class WaylandWindow : public RendererWindow<WaylandWindow, wayland::Window> {
  public:
    /** Bring the font to the class' namespace so that the RendererWindow can
     * find it.
     */
    using Font = WaylandFont;

    ~WaylandWindow() override;

    void setTitle(std::string const& value) override;

    void setIcon(Window::Icon icon) override;

    void setFullscreen(bool value = true) override;

    void show(bool value = true) override;

    void resize(Size const& newSize) override { MARK_AS_UNUSED(newSize); }

    void close() override {}

    void schedule(std::function<void()> event, Widget* widget) override;

  protected:
    /** Sets window cursor.
     */
    void setMouseCursor(MouseCursor cursor) override { MARK_AS_UNUSED(cursor); }

    void render(Rect const& rect) override { MARK_AS_UNUSED(rect); }

    void windowResized(int width, int height) override {
        MARK_AS_UNUSED(width);
        MARK_AS_UNUSED(height);
    }

    /** Handles FocusIn message sent to the window.
     */
    void focusIn() override {}

    /** Handles FocusOut message sent to the window.
     */
    void focusOut() override {}

    void requestClipboard(Widget* sender) override;

    void requestSelection(Widget* sender) override;

    void setClipboard(std::string const& contents) override;

    void setSelection(std::string const& contents, Widget* owner) override;

    void clearSelection(Widget* sender) override;

  private:
    friend class WaylandApplication;
    friend class RendererWindow<WaylandWindow, wayland::Window>;

    class MotifHints {
      public:
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long inputMode;
        unsigned long status;
    };

    /** Creates the renderer window of appropriate size using the default font
     * and zoom of 1.0.
     */
    WaylandWindow(std::string const& title, int cols, int rows,
                  EventQueue& eventQueue);

    /** \name Rendering Functions
     */
    //@{
    void initializeDraw() {}

    void finalizeDraw() {}

    void initializeGlyphRun(int col, int row) {
        MARK_AS_UNUSED(col);
        MARK_AS_UNUSED(row);
    }

    void addGlyph(int col, int row, Cell const& cell) {
        MARK_AS_UNUSED(col);
        MARK_AS_UNUSED(row);
        MARK_AS_UNUSED(cell);
    }

    /** Updates the current font.
     */
    void changeFont(ui::Font font) {
        font_ = WaylandFont::Get(font, cellSize_);
    }

    /** Updates the foreground color.
     */
    void changeForegroundColor(Color color) { MARK_AS_UNUSED(color); }

    /** Updates the background color.
     */
    void changeBackgroundColor(Color color) { MARK_AS_UNUSED(color); }

    /** Updates the decoration color.
     */
    void changeDecorationColor(Color color) { MARK_AS_UNUSED(color); }

    /** Draws the glyph run.

        First clears the background with given background color, then draws the
       text and finally applies any decorations.
     */
    void drawGlyphRun() {}

    /** Draws the border.

        Since the border is rendered over the contents and its color may be
       transparent, we can't use Xft's drawing, but have to revert to XRender
       which does the blending properly.
     */
    void drawBorder(int col, int row, Border const& border, int widthThin,
                    int widthThick) {
        MARK_AS_UNUSED(col);
        MARK_AS_UNUSED(row);
        MARK_AS_UNUSED(border);
        MARK_AS_UNUSED(widthThin);
        MARK_AS_UNUSED(widthThick);
    }
    //@}

    wayland::Window window_;

    WaylandFont* font_;

}; // tpp::WaylandWindow

} // namespace tpp

#endif
