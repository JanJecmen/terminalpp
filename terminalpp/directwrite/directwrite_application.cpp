#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)

#include <iostream>


#include "helpers/helpers.h"
#include "helpers/filesystem.h"

#include "windows.h"

#include "directwrite_application.h"
#include "directwrite_window.h"

namespace tpp {

	wchar_t const * const DirectWriteApplication::WindowClassName_ = L"TppWindowClass";
    wchar_t const * const DirectWriteApplication::DummyWindowName_ = L"dummy";

    void DirectWriteApplication::alert(std::string const & message) {
        utf16_string text{UTF8toUTF16(message)};
        MessageBox(nullptr, text.c_str(), L"t++", MB_ICONEXCLAMATION | MB_TASKMODAL);
    }

    bool DirectWriteApplication::query(std::string const & title, std::string const & message) {
        utf16_string caption{UTF8toUTF16(title)};
        utf16_string text{UTF8toUTF16(message)};
        return MessageBox(nullptr, text.c_str(), caption.c_str(), MB_ICONQUESTION | MB_TASKMODAL | MB_YESNOCANCEL) == IDYES;
    }

    void DirectWriteApplication::openLocalFile(std::string const & filename, bool edit) {
        utf16_string f{UTF8toUTF16(filename)};
        HINSTANCE result = ShellExecute(
            0, // handle to parent window, null since we want own process
            edit ? L"edit" : nullptr, // what to do with the file - this will choose the default action
            f.c_str(), // the file to open
            0, // no parameters since the local file is not an executable
            0, // working directory - leave the same as us for now
            SW_SHOWDEFAULT // whatever is the action for the associated program
        );
        // a bit ugly error checking (Win16 backwards compatribility as per MSDN)
        #pragma warning(push)
        #pragma warning(disable: 4302 4311)
        if ((int)result <= 32) {
            if (edit && (int)result == SE_ERR_NOASSOC)
                openLocalFile(filename, false);
            else
                alert(STR("Unable to determine proper viewer for file: " << filename));
        } 
        #pragma warning(pop)
    }

    void DirectWriteApplication::openUrl(std::string const & url) {
        utf16_string u{UTF8toUTF16(url)};
        ShellExecute(0, nullptr, u.c_str(), 0, 0, SW_SHOW);            
    }

    void DirectWriteApplication::setClipboard(std::string const & contents) {
		if (OpenClipboard(nullptr)) {
			EmptyClipboard();
			// encode the string into UTF16 and get the size of the data we need
			// ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
			utf16_string str = UTF8toUTF16(contents);
			// the str is null-terminated
			size_t size = (str.size() + 1) * 2;
			HGLOBAL clip = GlobalAlloc(0, size);
			if (clip) {
				WCHAR* data = reinterpret_cast<WCHAR*>(GlobalLock(clip));
				if (data) {
					memcpy(data, str.c_str(), size);
					GlobalUnlock(clip);
					SetClipboardData(CF_UNICODETEXT, clip);
				}
			}
			CloseClipboard();
		}

    }


	Window * DirectWriteApplication::createWindow(std::string const & title, int cols, int rows) {
		return new DirectWriteWindow(title, cols, rows);
	}

    void DirectWriteApplication::mainLoop() {
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
		}
    }

    DirectWriteApplication::DirectWriteApplication(HINSTANCE hInstance):
        hInstance_{hInstance},
        selectionOwner_{nullptr} {
        AttachConsole();
		// load the icons from the resource file
		iconDefault_ = LoadIcon(hInstance_, L"IDI_ICON1");
		iconNotification_ = LoadIcon(hInstance_, L"IDI_ICON2");
        registerWindowClass();
        registerDummyClass();
		D2D1_FACTORY_OPTIONS options;
		ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));
		OSCHECK(SUCCEEDED(D2D1CreateFactory(
			D2D1_FACTORY_TYPE_MULTI_THREADED,
			_uuidof(ID2D1Factory),
			&options,
			&d2dFactory_
		))) << "Unable to create D2D factory";
		OSCHECK(SUCCEEDED(DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			&dwFactory_
		))) << "Unable to create DW factory";
		// get the default user locale
		OSCHECK(SUCCEEDED(GetUserDefaultLocaleName(localeName_, LOCALE_NAME_MAX_LENGTH)));
		// get system font fallback
		OSCHECK(SUCCEEDED(static_cast<IDWriteFactory2*>(dwFactory_.Get())->GetSystemFontFallback(&fontFallback_))) << "Unable to create font fallback";
		// and get system font collection
		OSCHECK(SUCCEEDED(dwFactory_->GetSystemFontCollection(&systemFontCollection_, false))) << "Unable to get system font collection";
		// start the blinker thread
		DirectWriteWindow::StartBlinkerThread();

        dummy_ = CreateWindowExW(
            WS_EX_LEFT, // the default
            DummyWindowName_, // window class
            // ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
            L"dummy", // window name (all start as terminal++)
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, // x position
            CW_USEDEFAULT, // y position
            0,
            0,
            nullptr, // handle to parent
            nullptr, // handle to menu 
            hInstance_, // module handle
            this // lParam for WM_CREATE message
        );
    }

    void DirectWriteApplication::registerWindowClass() {
		WNDCLASSEXW wClass = { 0 };
		wClass.cbSize = sizeof(WNDCLASSEX); // size of the class info
		wClass.hInstance = hInstance_;
		wClass.style = CS_HREDRAW | CS_VREDRAW;
		wClass.lpfnWndProc = DirectWriteWindow::EventHandler; // event handler function for the window class
		wClass.cbClsExtra = 0; // extra memory to be allocated for the class
		wClass.cbWndExtra = 0; // extra memory to be allocated for each window
		wClass.lpszClassName = WindowClassName_; // class name
		wClass.lpszMenuName = nullptr; // menu name
		wClass.hIcon = iconDefault_; // big icon (alt-tab)
		wClass.hIconSm = iconDefault_; // small icon (taskbar)
		wClass.hCursor = LoadCursor(nullptr, IDC_IBEAM); // mouse pointer icon
		wClass.hbrBackground = nullptr; // do not display background - the terminal window does it itself
		// register the class
		OSCHECK(RegisterClassExW(&wClass) != 0) << "Unable to register window class";
    }

    void DirectWriteApplication::registerDummyClass() {
		WNDCLASSEXW wClass = { 0 };
		wClass.cbSize = sizeof(WNDCLASSEX); // size of the class info
		wClass.hInstance = hInstance_;
		wClass.style = CS_HREDRAW | CS_VREDRAW;
		wClass.lpfnWndProc = DirectWriteApplication::UserEventHandler_; // event handler function for the window class
		wClass.cbClsExtra = 0; // extra memory to be allocated for the class
		wClass.cbWndExtra = 0; // extra memory to be allocated for each window
		wClass.lpszClassName = DummyWindowName_; // class name
		wClass.lpszMenuName = nullptr; // menu name
		wClass.hbrBackground = nullptr; // do not display background
		OSCHECK(RegisterClassExW(&wClass) != 0) << "Unable to register window class";
    }

	LRESULT CALLBACK DirectWriteApplication::UserEventHandler_(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_USER:
                DirectWriteWindow::ProcessEvent();
                break;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

} // namespace tpp

#endif 
