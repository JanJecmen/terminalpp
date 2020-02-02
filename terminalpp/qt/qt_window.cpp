#if (defined RENDERER_QT)
#include "qt_window.h"

namespace tpp {

    QtWindow::~QtWindow() {

    }

    void QtWindow::show() {
        QOpenGLWindow::show();
    }

    /** Sets the window icon. 
     */
    void QtWindow::setIcon(ui::RootWindow::Icon icon) {
        MARK_AS_UNUSED(icon);
    }

    QtWindow::QtWindow(std::string const & title, int cols, int rows, unsigned baseCellHeightPx):
        RendererWindow(cols, rows, QtFont::GetOrCreate(ui::Font(), 0, baseCellHeightPx)->widthPx(), baseCellHeightPx),
        font_{ nullptr }
        {
        QOpenGLWindow::resize(widthPx_, heightPx_);
        QOpenGLWindow::setTitle(title.c_str());


        connect(this, SIGNAL(tppRequestUpdate()), this, SLOT(update()), Qt::ConnectionType::QueuedConnection);
        AddWindowNativeHandle(this, this);
    }

    void QtWindow::keyPressEvent(QKeyEvent* ev) {
        static_assert(sizeof(QChar) == sizeof(helpers::utf16_char));
        activeModifiers_ = GetStateModifiers(ev->modifiers());
        ui::Key k{GetKey(ev->key(), activeModifiers_.modifiers(), true)};
        if (k != ui::Key::Invalid)
            keyDown(k);
        // determine if there is a printable character to be sent
        QString str{ev->text()};
        if (!str.isEmpty()) {
            helpers::utf16_char const * data = pointer_cast<helpers::utf16_char const*>(str.data());
            helpers::Char c{helpers::Char::FromUTF16(data, data + str.size())};
            // raise the keyChar event
            // TODO what to do with delete
            if (c.codepoint() >= 32)
                keyChar(c);
        }
    }

    void QtWindow::keyReleaseEvent(QKeyEvent* ev) {
        activeModifiers_ = GetStateModifiers(ev->modifiers());
        ui::Key k{GetKey(ev->key(), activeModifiers_.modifiers(), false)};
        if (k != ui::Key::Invalid)
            keyUp(k);

    }

    void QtWindow::requestClipboardContents() {
        NOT_IMPLEMENTED;
    }

    void QtWindow::requestSelectionContents() {
        NOT_IMPLEMENTED;
    }

    void QtWindow::setClipboard(std::string const & contents) {
        MARK_AS_UNUSED(contents);
        NOT_IMPLEMENTED;
    }

    void QtWindow::setSelection(std::string const & contents) {
        MARK_AS_UNUSED(contents);
        NOT_IMPLEMENTED;
    }

    void QtWindow::clearSelection() {
        NOT_IMPLEMENTED;
    }

    unsigned QtWindow::GetStateModifiers(Qt::KeyboardModifiers const & modifiers) {
        unsigned result = 0;
        if (modifiers & Qt::KeyboardModifier::ShiftModifier)
            result += ui::Key::Shift;
        if (modifiers & Qt::KeyboardModifier::ControlModifier)
            result += ui::Key::Ctrl;
        if (modifiers & Qt::KeyboardModifier::AltModifier)
            result += ui::Key::Alt;
        if (modifiers & Qt::KeyboardModifier::MetaModifier)
            result += ui::Key::Win;
        return result;
    }


    // TODO check if shift meta and friends must do anything with the pressed
    ui::Key QtWindow::GetKey(int qtKey, unsigned modifiers, bool pressed) {
        if (qtKey >= Qt::Key::Key_A && qtKey <= Qt::Key::Key_Z) 
            return ui::Key{static_cast<unsigned>(qtKey), modifiers};
        if (qtKey <= Qt::Key::Key_0 && qtKey <= Qt::Key::Key_9) 
            return ui::Key{static_cast<unsigned>(qtKey), modifiers};
        switch (qtKey) {
            case Qt::Key::Key_Backspace:
                return ui::Key{ui::Key::Backspace, modifiers};
            case Qt::Key::Key_Tab:
                return ui::Key{ui::Key::Tab, modifiers};
            case Qt::Key::Key_Enter:
            case Qt::Key::Key_Return:
                return ui::Key{ui::Key::Enter, modifiers};
            case Qt::Key::Key_Shift:
                return ui::Key{ui::Key::ShiftKey, modifiers};
            case Qt::Key::Key_Control:
                return ui::Key{ui::Key::CtrlKey, modifiers};
            case Qt::Key::Key_Alt:
            case Qt::Key::Key_AltGr:
                return ui::Key{ui::Key::AltKey, modifiers};
            case Qt::Key::Key_CapsLock:
                return ui::Key{ui::Key::CapsLock, modifiers};
            case Qt::Key::Key_Escape:
                return ui::Key{ui::Key::Esc, modifiers};
            case Qt::Key::Key_Space:
                return ui::Key{ui::Key::Space, modifiers};
            case Qt::Key::Key_PageUp:
                return ui::Key{ui::Key::PageUp, modifiers};
            case Qt::Key::Key_PageDown:
                return ui::Key{ui::Key::PageDown, modifiers};
            case Qt::Key::Key_End:
                return ui::Key{ui::Key::End, modifiers};
            case Qt::Key::Key_Home:
                return ui::Key{ui::Key::Home, modifiers};
            case Qt::Key::Key_Left:
                return ui::Key{ui::Key::Left, modifiers};
            case Qt::Key::Key_Up:
                return ui::Key{ui::Key::Up, modifiers};
            case Qt::Key::Key_Right:
                return ui::Key{ui::Key::Right, modifiers};
            case Qt::Key::Key_Down:
                return ui::Key{ui::Key::Down, modifiers};
            case Qt::Key::Key_Insert:
                return ui::Key{ui::Key::Insert, modifiers};
            case Qt::Key::Key_Delete:
                return ui::Key{ui::Key::Delete, modifiers};
            case Qt::Key::Key_Meta:
                return ui::Key{ui::Key::WinKey, modifiers};
            case Qt::Key::Key_Menu:
                return ui::Key{ui::Key::Menu, modifiers};
            // TODO deal with numeric keyboard
            case Qt::Key::Key_F1:
                return ui::Key{ui::Key::F1, modifiers};
            case Qt::Key::Key_F2:
                return ui::Key{ui::Key::F2, modifiers};
            case Qt::Key::Key_F3:
                return ui::Key{ui::Key::F3, modifiers};
            case Qt::Key::Key_F4:
                return ui::Key{ui::Key::F4, modifiers};
            case Qt::Key::Key_F5:
                return ui::Key{ui::Key::F5, modifiers};
            case Qt::Key::Key_F6:
                return ui::Key{ui::Key::F6, modifiers};
            case Qt::Key::Key_F7:
                return ui::Key{ui::Key::F7, modifiers};
            case Qt::Key::Key_F8:
                return ui::Key{ui::Key::F8, modifiers};
            case Qt::Key::Key_F9:
                return ui::Key{ui::Key::F9, modifiers};
            case Qt::Key::Key_F10:
                return ui::Key{ui::Key::F10, modifiers};
            case Qt::Key::Key_F11:
                return ui::Key{ui::Key::F11, modifiers};
            case Qt::Key::Key_F12:
                return ui::Key{ui::Key::F12, modifiers};
            case Qt::Key::Key_NumLock:
                return ui::Key{ui::Key::NumLock, modifiers};
            case Qt::Key::Key_ScrollLock:
                return ui::Key{ui::Key::ScrollLock, modifiers};
            case Qt::Key::Key_Semicolon:
                return ui::Key{ui::Key::Semicolon, modifiers};
            case Qt::Key::Key_Equal:
                return ui::Key{ui::Key::Equals, modifiers};
            case Qt::Key::Key_Comma:
                return ui::Key{ui::Key::Comma, modifiers};
            case Qt::Key::Key_Minus:
                return ui::Key{ui::Key::Minus, modifiers};
            case Qt::Key::Key_Period:
                return ui::Key{ui::Key::Dot, modifiers};
            case Qt::Key::Key_Slash:
                return ui::Key{ui::Key::Slash, modifiers};
            // TODO tick (tilde)
            case Qt::Key::Key_BraceLeft:
                return ui::Key{ui::Key::SquareOpen, modifiers};
            case Qt::Key::Key_Backslash:
                return ui::Key{ui::Key::Backslash, modifiers};
            case Qt::Key::Key_BraceRight:
                return ui::Key{ui::Key::SquareClose, modifiers};
            // TODO Quote
            default:
                return ui::Key{ui::Key::Invalid, modifiers};
        }
    }

} // namespace tpp

#endif
