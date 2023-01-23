#pragma once
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>

#pragma comment (lib, "Ws2_32.lib")



#ifndef NDEBUG
#include <sstream>
#define DEBUGTRACELINE(X)						\
	do {	std::stringstream ss;  \
			ss << __FILE__ << "(" << __LINE__ << ") : \n";  \
			ss << X << std::endl;; \
			OutputDebugStringA(ss.str().c_str());	\
			} while(0)

#define DEBUGTRACE(X)							\
	do {	std::stringstream wss; wss << X << "\n";	\
			OutputDebugStringA(wss.str().c_str());	\
				} while(0)

#else
#define DEBUGTRACE(X)
#define DEBUGTRACELINE(X)
#endif

#define ERROREXIT(MSG, CODE) \
	do {	MessageBox(NULL, MSG, "WIN SSH-AGENT for MSYS2", MB_OK);	\
			exit(CODE); \
	}	while (0)