﻿#include <iostream>

#include "helpers/log.h"

#include "vterm/local_pty.h"
#include "vterm/vt100.h"

#include "config.h"
#include "session.h"

#ifdef WIN32
#include "gdi/gdi_application.h"
#include "gdi/gdi_terminal_window.h"
// link to directwrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#elif __linux__
#include "x11/x11_application.h"
#include "x11/x11_terminal_window.h"
#else
#error "Unsupported platform"
#endif



using namespace tpp;

// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

// https://github.com/Microsoft/node-pty/blob/master/src/win/conpty.cc



/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	try {
	    // create the application singleton
	    new GDIApplication(hInstance);
#elif __linux__
int main(int argc, char* argv[]) {
	try {
	    // create the application singleton
	    new X11Application();
#endif

		Session* s = new Session("t++", DEFAULT_SESSION_COMMAND);
		s->start();
		s->show();

    	Application::MainLoop();

	    return EXIT_SUCCESS;
	} catch (helpers::Exception const& e) {
		std::cout << e;
	} catch (std::exception const& e) {
		std::cout << e.what();
	} catch (...) {
		std::cout << "unknown error";
	}
	return EXIT_FAILURE;
}
