#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include <atomic>

#include "../application.h"
#include "wayland.h"

namespace tpp {

class WaylandWindow;

class WaylandApplication : public Application {
  public:
    static void Initialize(int argc, char** argv) {
        MARK_AS_UNUSED(argc);
        MARK_AS_UNUSED(argv);
        new WaylandApplication();
    }

    static WaylandApplication* Instance() {
        return dynamic_cast<WaylandApplication*>(Application::Instance());
    }

    ~WaylandApplication() override;

    /** Displays a GUI alert.

        Because X11 does not have a simple function to display a message box,
       the method cheats and calls the `xmessage` command with the message as an
       argument which should display the message window anyways.

        In the unlikely case that `xmessage` command is not found, the error
       message will be written to the stdout as a last resort.
     */
    void alert(std::string const& message) override;

    bool query(std::string const& title, std::string const& message) override;

    /** Opens the given local file using the default viewer/editor.

        Internally, `xdg-open` is used to determine the file to open. If edit is
       true, then default system editor will be launched inside the default x
       terminal.

        TODO this is perhaps not the best option, but works fine-ish for now and
       linux default programs are a bit of a mess if tpp were to support it all.
     */
    void openLocalFile(std::string const& filename, bool edit) override;

    void openUrl(std::string const& url) override;

    /** Sets the clipboard contents.

        This is not trivial if the main loop is not running because setting the
       clipboard actually means waiting for the clipboard manager to ask for its
       contents. To do so, the function detects if main loop is running, and if
       not cherrypicks the incomming clipboard messages for up to
       SET_CLIPBOARD_TIMEOUT milliseconds.

        TODO for now waits the entire timeout because terminating immediately
       after the clipboard contents has been requested by the clipboard manager
       did not actually send the data properly. It's ok as setting clipboard
       outside of the main loop is a cornercase.
     */
    void setClipboard(std::string const& contents) override;

    Window* createWindow(std::string const& title, int cols, int rows) override;

    void mainLoop() override;

  private:
    friend class WaylandWindow;

    class TerminateException {};

    WaylandApplication();

    std::atomic<bool> mainLoopRunning_;

    /** Font config state.
     */
    FcConfig* fcConfig_;

    std::string clipboard_;
    std::string selection_;
    WaylandWindow* selectionOwner_;

}; // WaylandApplication

} // namespace tpp

#endif
