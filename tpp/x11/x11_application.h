#pragma once
#ifdef ARCH_UNIX

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "x11.h"
#include "../application.h"

namespace tpp {

	class X11TerminalWindow;

    /** https://www.student.cs.uwaterloo.ca/~cs349/f15/resources/X/xTutorialPart1.html
     */
	class X11Application : public Application {
	public:
		X11Application();
		~X11Application() override;

		TerminalWindow * createTerminalWindow(Session * session, TerminalWindow::Properties const& properties, std::string const& name) override;

		std::pair<unsigned, unsigned> terminalCellDimensions(unsigned fontSize) override;

        /** Sends given X event. 

            Because Xlib is not great with multiple threads, XFlush must be called after each event being set programatically to the queue. 
         */
        void xSendEvent(X11TerminalWindow * window, XEvent & e, long mask = 0);

		Display* xDisplay() {
			return xDisplay_;
		}

		int xScreen() {
			return xScreen_;
		}

	protected:

		void openInputMethod();

		void mainLoop() override;

		void sendFPSTimerMessage() override {
			XEvent e;
			memset(&e, 0, sizeof(XEvent));
			e.xclient.type = ClientMessage;
			e.xclient.display = xDisplay_;
			e.xclient.window = x11::None;
			e.xclient.format = 32;
			e.xclient.data.l[0] = fpsTimerMessage_;
			xSendEvent(nullptr, e);
		}
	private:
		friend class X11TerminalWindow;

		/** An exception to be thrown when the program should terminate. 
		 */
		class Terminate { };

		/** X11 display. */
	    Display* xDisplay_;
		int xScreen_;
		/** A window that always exists, is always hidden and we use it to send broadcast messages because X does not allow window-less messages and this feels simpler than copying the whole queue. 
		 */
		Window broadcastWindow_;
        XIM xIm_;
		Atom wmDeleteMessage_;
		Atom fpsTimerMessage_;
        Atom primaryName_;
		Atom clipboardName_;
		Atom formatString_;
		Atom formatStringUTF8_;
		Atom formatTargets_;
		Atom clipboardIncr_;
		Atom motifWmHints_;
		Atom netWmIcon_;
		std::string clipboard_;

	}; // tpp::X11Application

	/** Because XFT font sizes are ascent only, the font is obtained by trial and error. First we try the requested height and then, based on the actual height. If the actually obtained height differs, height multiplier is calculated and the font is re-obtained with the height adjusted. */
	template<>
	inline FontSpec<XftFont*>* FontSpec<XftFont*>::Create(vterm::Font font, unsigned height) {
		X11Application* app = reinterpret_cast<X11Application*>(Application::Instance());
		// get the name of the font we want w/o the actual height
		std::string fName = *config::FontFamily;
		if (font.bold())
			fName += ":bold";
		if (font.italics())
			fName += ":italic";
		fName += ":pixelsize=";
		// get the font we request
		std::string fRequest = STR(fName << height);
		XftFont* handle = XftFontOpenName(app->xDisplay(), app->xScreen(), fRequest.c_str());
		// if the size is not correct, adjust height and try again
		if (static_cast<unsigned>(handle->ascent + handle->descent) != height) {
			double hAdj = height * static_cast<double>(height) / (handle->ascent + handle->descent);
			XftFontClose(app->xDisplay(), handle);
			fRequest = STR(fName << hAdj);
			handle = XftFontOpenName(app->xDisplay(), app->xScreen(), fRequest.c_str());
		}
		// get the font width
		XGlyphInfo gi;
		XftTextExtentsUtf8(app->xDisplay(), handle, (FcChar8*)"m", 1, &gi);
		// return the font 
		return new FontSpec<XftFont*>(font, gi.width, height, handle);
	}

	template<>
	inline FontSpec<XftFont*>::~FontSpec<XftFont*>() {
		XftFontClose(Application::Instance<X11Application>()->xDisplay(), handle_);
	}


}

#endif
