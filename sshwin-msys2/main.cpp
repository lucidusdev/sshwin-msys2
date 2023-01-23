#include <string>
#include <thread>
#include <regex>

#include <cstdlib>

#include "LimitSingleInstance.h"
#include "socketServer.h"
#include "pch.h"

#define WKCLASS_MAIN "MSYS2-WINSSH-AGENT"

namespace {
	CLimitSingleInstance g_SingleInstanceObj("Global\\{211D640F-BEE8-4F10-B7AF-3B0E506BC401}");
}


LRESULT CALLBACK WndMsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_USER + 1) {
		PostQuitMessage(0);
		return 0;

	}

	return DefWindowProc(hwnd, msg, wParam, lParam);

}

void threadRunner(socketServer ss) {
	ss.Start();
}

int main(int argc, char* argv[])
{

	std::string port = "61000";
	std::string socketFile = "C:\\MSYS64\\tmp\\sshwin-msys2.sock";
	std::string pipeName = "openssh-ssh-agent";
	std::string hostIP = "127.0.0.1";
	int timeout = 60000;  //60s conn timeout
	bool bReload = false;
	int delayStart = 0;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-p") && i + 1 < argc) {
			port = argv[i + 1];
			i++;
		}
		else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
			socketFile = argv[i + 1];
			i++;
		}
		else if (!strcmp(argv[i], "-n") && i + 1 < argc) {
			pipeName = argv[i + 1];
			i++;
		}
		else if (!strcmp(argv[i], "-t") && i + 1 < argc) {
			timeout = max(5000, stoi(std::string(argv[i + 1]))); //in ms, min 5000
			i++;
		}
		else if (!strcmp(argv[i], "-h") && i + 1 < argc) {
			hostIP = argv[i + 1];  //host ip
			i++;
		}
		else if (!strcmp(argv[i], "-r")) {
			bReload = true;
		}

	}

	if (g_SingleInstanceObj.IsAnotherInstanceRunning())
	{
		if (bReload ||
			MessageBoxA(NULL, "Win ssh-agent<->Msys2 is running\nOK to stop, Cancel to keep.", "SSHWIN-MSYS2 by lex-c@github", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
		{
		//_MessageBoxOK(L"Another instance is running, exiting...");
			HWND h = FindWindow(WKCLASS_MAIN, NULL);
			if (h)
				SendMessage(h, WM_USER + 1, 0, 0);
		}
		if (!bReload)
			return 0;
	}

	auto hInst = GetModuleHandle(NULL);
	WNDCLASSEX wx = { sizeof(wx) };
	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = WndMsgProc;        //msg loop
	wx.hInstance = hInst;
	wx.lpszClassName = WKCLASS_MAIN;
	if (RegisterClassEx(&wx)) {
		CreateWindowEx(0, WKCLASS_MAIN, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	}

	//create server thread and handle everything
	socketServer::serverConfig serverConfig;
	serverConfig.IP = hostIP;
	serverConfig.port = port;
	serverConfig.socketFile = socketFile;
	serverConfig.pipeName = pipeName; 
	serverConfig.timeout = timeout; //min 2 seconds
	DEBUGTRACELINE(socketFile << " " << port << " " << pipeName << " " << timeout);

	socketServer myServer(serverConfig);
	std::thread thread(threadRunner, myServer);
	thread.detach();

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	UnregisterClass(WKCLASS_MAIN, hInst);

}


int APIENTRY WinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	return main(__argc, __argv);
}