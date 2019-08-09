#include <memory>

#include "helpers/string.h"

#include "local_pty.h"


namespace vterm {

#if (defined ARCH_WINDOWS)

    LocalPTY::LocalPTY(helpers::Command const& command):
        command_{command},
   		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
        start();
    }

	LocalPTY::LocalPTY(helpers::Command const& command, helpers::Environment const & env) :
		command_(command),
		environment_(env),
		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
        start();
    }

    LocalPTY::~LocalPTY() {
        terminate();
    }

    // PTY interface

    void LocalPTY::send(char const * buffer, size_t bufferSize) {
		DWORD bytesWritten = 0;
		size_t start = 0;
		size_t i = 0;
		while (i < bufferSize) {
			if (buffer[i] == '`') {
				WriteFile(pipeOut_, buffer + start, static_cast<DWORD>(i + 1 - start), &bytesWritten, nullptr);
				start = i;
			}
			++i;
		}
		WriteFile(pipeOut_, buffer + start, static_cast<DWORD>(i - start), &bytesWritten, nullptr);
    }

    size_t LocalPTY::receive(char * buffer, size_t bufferSize) {
		DWORD bytesRead = 0;
		ReadFile(pipeIn_, buffer, static_cast<DWORD>(bufferSize), &bytesRead, nullptr);
		return bytesRead;
    }

    void LocalPTY::terminate() {
        if (TerminateProcess(pInfo_.hProcess, std::numeric_limits<unsigned>::max()) != ERROR_SUCCESS) {
            // it could be that the process has already terminated
    		helpers::ExitCode ec = STILL_ACTIVE;
	    	GetExitCodeProcess(pInfo_.hProcess, &ec);
            // the process has been terminated already, it's ok
            if (ec != STILL_ACTIVE)
                return;
            // otherwise throw the last error (this could in theory be error from the GetExitCodeProcess, but that's ok for now)
            OSCHECK(false); 
        }
    }

    helpers::ExitCode LocalPTY::waitFor() {
        OSCHECK(WaitForSingleObject(pInfo_.hProcess, INFINITE) != WAIT_FAILED);
		helpers::ExitCode ec;
		OSCHECK(GetExitCodeProcess(pInfo_.hProcess, &ec) != 0);
        return ec;
    }

    void LocalPTY::resize(int cols, int rows) {
		// resize the underlying ConPTY
		COORD size;
		size.X = cols & 0xffff;
		size.Y = rows & 0xffff;
		ResizePseudoConsole(conPTY_, size);
    }

    void LocalPTY::start() {
		STARTUPINFOEX startupInfo{};
        // create the pseudoconsole
		HRESULT result{ E_UNEXPECTED };
		HANDLE pipePTYIn{ INVALID_HANDLE_VALUE };
		HANDLE pipePTYOut{ INVALID_HANDLE_VALUE };
		// first create the pipes we need, no security arguments and we use default buffer size for now
		OSCHECK(
			CreatePipe(&pipePTYIn, &pipeOut_, NULL, 0) && CreatePipe(&pipeIn_, &pipePTYOut, NULL, 0)
		) << "Unable to create pipes for the subprocess";
		// determine the console size from the terminal we have
		COORD consoleSize{};
		consoleSize.X = 80;
		consoleSize.Y = 25;
		// now create the pseudo console
		result = CreatePseudoConsole(consoleSize, pipePTYIn, pipePTYOut, 0, &conPTY_);
		// delete the pipes on PTYs end, since they are now in conhost and will be deleted when the conpty is deleted
		if (pipePTYIn != INVALID_HANDLE_VALUE)
			CloseHandle(pipePTYIn);
		if (pipePTYOut != INVALID_HANDLE_VALUE)
			CloseHandle(pipePTYOut);
		OSCHECK(result == S_OK) << "Unable to open pseudo console";
        // generate the startup info
		size_t attrListSize = 0;
		startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
		// allocate the attribute list of required size
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize); // get size of list of 1 attribute
        std::unique_ptr<char> attrList(new char[attrListSize]);        
		startupInfo.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrList.get());
		// initialize the attribute list
		OSCHECK(
			InitializeProcThreadAttributeList(startupInfo.lpAttributeList, 1, 0, &attrListSize)
		) << "Unable to create attribute list";
		// set the pseudoconsole attribute
		OSCHECK(
			UpdateProcThreadAttribute(
				startupInfo.lpAttributeList,
				0,
				PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
				conPTY_,
				sizeof(HPCON),
				nullptr,
				nullptr
			)
		) << "Unable to set pseudoconsole attribute";
		// finally, create the process with given commandline
		helpers::utf16_string cmd = helpers::UTF8toUTF16(command_.toString());
		OSCHECK(
			CreateProcess(
				nullptr,
				&cmd[0], // the command to execute
				nullptr, // process handle cannot be inherited
				nullptr, // thread handle cannot be inherited
				false, // the new process does not inherit any handles
				EXTENDED_STARTUPINFO_PRESENT, // we have extra info 
				nullptr, // use parent's environment
				nullptr, // use parent's directory
				&startupInfo.StartupInfo, // startup info
				&pInfo_ // info about the process
			)
		) << "Unable to start process " << command_;
    }

#endif


} // namespace vterm