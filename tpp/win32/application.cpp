#ifdef WIN32
#include <iostream>

#include <windows.h>

#include "helpers/win32.h"

#include "application.h"
#include "terminal_window.h"


namespace tpp {
	namespace {
		/** Attaches a console to the application for debugging purposes. 
		 */
		void AttachConsole() {
			if (AllocConsole() == 0)
				THROW(helpers::Win32Error("Cannot allocate console"));
			// this is ok, console cannot be detached, so we are fine with keeping the file handles forewer,
			// nor do we need to FreeConsole at any point
			FILE *fpstdin = stdin, *fpstdout = stdout, *fpstderr = stderr;
			// patch the cin, cout, cerr
			freopen_s(&fpstdin, "CONIN$", "r", stdin);
			freopen_s(&fpstdout, "CONOUT$", "w", stdout);
			freopen_s(&fpstderr, "CONOUT$", "w", stderr);
			std::cin.clear();
			std::cout.clear();
			std::cerr.clear();
		}


	} // anonymous namespace

	char const * const Application::TerminalWindowClassName_ = "TerminalWindowClass";

	Application::Application(HINSTANCE hInstance):
	    hInstance_(hInstance) {
		// TODO this should be conditional on a debug flag
		AttachConsole();
		registerTerminalWindowClass();
	}

	Application::~Application() {

	}

	void Application::mainLoop() {
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// here we are in the on idle mode so we can process queues of all terminals. 
	}

	void Application::registerTerminalWindowClass() {
		WNDCLASSEX wClass = { 0 };
		wClass.cbSize = sizeof(WNDCLASSEX); // size of the class info
		wClass.hInstance = hInstance_;
		wClass.style = CS_HREDRAW | CS_VREDRAW;
		wClass.lpfnWndProc = TerminalWindow::EventHandler; // event handler function for the window class
		wClass.cbClsExtra = 0; // extra memory to be allocated for the class
		wClass.cbWndExtra = 0; // extra memory to be allocated for each window
		wClass.lpszClassName = TerminalWindowClassName_; // class name
		wClass.lpszMenuName = nullptr; // menu name
		wClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // big icon (alt-tab)
		wClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION); // small icon (taskbar)
		wClass.hCursor = LoadCursor(nullptr, IDC_IBEAM); // mouse pointer icon
		wClass.hbrBackground = nullptr; // do not display background - the terminal window does it itself
		// register the class
		ATOM result = RegisterClassEx(&wClass);
		ASSERT(result != 0) << "Unable to register window class";
	}



} // namespace tpp

#endif