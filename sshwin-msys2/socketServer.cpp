#include <fstream>
#include <vector>
#include <regex>

#include <WS2tcpip.h>

#include "socketServer.h"
#include "pch.h"


socketServer::socketServer(serverConfig _config)
{

	sc = _config;
	//create file and save guid
	if (!sc.socketFile.empty() && sc.socketFile != "null")
		sc.guid = initMsys2Socket();
	DEBUGTRACELINE(sc.guid);
	return;
}

socketServer::~socketServer()
{
	return;
}

void socketServer::Start()
{
	if (proxyServer() == -1)
		ERROREXIT("Failed to start socket server", 1);
	return;
}

int socketServer::proxyServer()
{
	//initPipe();
	WSADATA WSAData = { 0 };

	if (WSAStartup(MAKEWORD(2, 0), &WSAData))
	{
		DEBUGTRACELINE("socket startup failed!");
		WSACleanup();
		return -1;
	}

	SOCKET sServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


	if (sServer == INVALID_SOCKET) {
		DEBUGTRACELINE("socket failed!");
		WSACleanup();
		return -1;
	}
	int timeout = sc.timeout;  //60000ms = 60s timeout for password input 
	if (setsockopt(sServer, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		DEBUGTRACELINE("Failed to set recv timeout");
		WSACleanup();
		return -1;
	}

	sockaddr_in addrserv;
	addrserv.sin_family = AF_INET;
	addrserv.sin_port = htons(stoi(sc.port));
	//addrserv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	InetPton(AF_INET, sc.IP.c_str(), &(addrserv.sin_addr.s_addr));

	if (bind(sServer, (const struct sockaddr*)&addrserv, sizeof(addrserv)) == SOCKET_ERROR) {
		DEBUGTRACELINE("bind failed!");
		closesocket(sServer);
		WSACleanup();
		return -1;
	}

	if (listen(sServer, 2) == SOCKET_ERROR) { //2 connections
		DEBUGTRACELINE("listen failed!");
		closesocket(sServer);
		WSACleanup();
		return -1;
	}
	SOCKET sClient;  //client

	sockaddr_in addrclient;
	int addrsize = sizeof(addrclient);

	char data[BUFLEN] = { 0 };
	//int workingport = -1;
	sshMessage msg;

	while (1)
	{
		DEBUGTRACELINE("Wait for new connection...");
		sClient = accept(sServer, (sockaddr*)&addrclient, &addrsize);

		if (sClient == INVALID_SOCKET) {
			DEBUGTRACELINE("accept failed!");
			//closesocket(sServer);
			//WSACleanup();
			//break;
			continue; //keep running?
		}
		int clientportin = ntohs(addrclient.sin_port);
		int iState = 1;
		DEBUGTRACELINE("accepted client port :" << clientportin);
		msg.reset();
		while (1)
		{
			int count = recv(sClient, data, BUFLEN, 0);
			DEBUGTRACELINE(">>>bytes received " << count);

			if (count <= 0)
			{
				DEBUGTRACELINE("...exit...with bytes " << count);
				goto CLEAN_UP;
			}
			if (iState < 3)
			{
				if (!initConnection(sClient, data, count, iState))
					goto CLEAN_UP;
			}
			if (iState == 3) 
			{
				if (count < 4 && msg.length == 0)  //new msg but less than 4 bytes
					goto CLEAN_UP;
				int offset = 0;
				if (msg.length == 0)
				{
					uint32_t val = 0;
					memcpy(&val, data, 4);
					msg.length = ntohl(val);
					msg.header.assign(data, data + 4);
					if (count == 4)
						continue;
					offset = 4;
					//DEBUGTRACELINE("Expect MSG length: " << msg.length << " Got: " << msg.content.size());
				}
	
				if (msg.length != 0)
				{
					msg.content.insert(msg.content.end(), data + offset, data + count);
					if (msg.content.size() == msg.length || count < BUFLEN) //all read
					{
						if (streamPipe(sClient, msg))
							msg.reset();
						else
							goto CLEAN_UP;
					}
				}
			}
			continue;
		CLEAN_UP:
			CloseHandle(sc.pipe);
			sc.pipe = INVALID_HANDLE_VALUE;
			closesocket(sClient);
			break;
		}
	}
	return 0;
}

bool socketServer::initPipe()
{
	if (sc.pipe == INVALID_HANDLE_VALUE)
	{
		std::string pname;
		if (!getPipename(pname))
		{
			DEBUGTRACELINE("No pipename---" << pname << "---" << sc.pipeName);
			return false;
		}
		sc.pipe = CreateFile(("\\\\.\\pipe\\" + pname).c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		DEBUGTRACELINE("pipe--" << pname << "--init with " << sc.pipe);
	}
	return (sc.pipe != INVALID_HANDLE_VALUE);

}

/// <summary>
/// Get actual pipe name 
/// Use regex match if it contains "*". Otherwise, just return input.
/// </summary>
/// <returns></returns>
bool socketServer::getPipename(std::string& pname)
{
	if (sc.pipeName.find("*") == std::string::npos) //no * included, good name.
	{
		pname = sc.pipeName;
		return true;
	}

	WIN32_FIND_DATA fd;
	bool found = false;
	HANDLE hFind = FindFirstFile("\\\\.\\pipe\\*", &fd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {
			std::string str(fd.cFileName);
			std::regex re(sc.pipeName);
			std::smatch r;
			if (std::regex_search(str, r, re))
			{
				pname = std::string(fd.cFileName);
				found = true;
				DEBUGTRACELINE("Found pipename " + pname);
				break;
			}
		} while (FindNextFile(hFind, &fd));
	}
	FindClose(hFind);
	return found;
}

/// <summary>
/// create socket file for msys2
/// </summary>
/// <returns></returns>
std::string socketServer::initMsys2Socket()
{
	//always create a new one
	std::string ret;
	SetFileAttributes(sc.socketFile.c_str(), FILE_ATTRIBUTE_NORMAL);
	std::ofstream output(sc.socketFile, std::ios::binary);
	if (output.fail()) //can't write
	{
		ERROREXIT(("Create " + sc.socketFile + " Failed!!\nProgram Exit.").c_str(), 1);
	}
	GUID _guid;
	if (S_OK == CoCreateGuid(&_guid))
	{
		char buff[40] = { 0 };
		snprintf(buff, 36, "%08X-%04X%04X-%02X%02X%02X%02X-%02X%02X%02X%02X",
			_guid.Data1, _guid.Data2, _guid.Data3, _guid.Data4[0], _guid.Data4[1], _guid.Data4[2], _guid.Data4[3], _guid.Data4[4], _guid.Data4[5], _guid.Data4[6], _guid.Data4[7]);
		char out[61] = { 0 };
		snprintf(out, 60, "!<socket >%s s %s", sc.port.c_str(), buff);
		output.write(out, strlen(out) + 1);
		ret = std::string(buff);
	}
	output.close();
	SetFileAttributes(sc.socketFile.c_str(), FILE_ATTRIBUTE_SYSTEM);

	return ret;
}


bool socketServer::initConnection(SOCKET sock, char* data, int count, int& iState)
{
	if (iState == 1)
	{
		if (count == 16 && !sc.guid.empty()) {
			int val[4];
			for (auto i = 0; i < 4; i++)
			{
				memcpy(&val[i], data + i * 4, 4);
			}
			char buff[40] = { 0 };
			snprintf(buff, 36, "%08X-%08X-%08X-%08X\0\0", val[0], val[1], val[2], val[3]);
			std::string guidin(buff);
			if (guidin != sc.guid) {
				DEBUGTRACELINE("NOT MATCH " + guidin + " " + sc.guid + "\nTest for generic ssh");
				goto GENERIC_MSG;
			}
			DEBUGTRACELINE("Send back " << count << " bytes");
			if (send(sock, data, count, 0) == SOCKET_ERROR)
			{
				DEBUGTRACELINE("Send error " << WSAGetLastError());
				return false;
			}
			iState = 2;
			return true;
		}
		GENERIC_MSG:
		if (count >=4) //at least 4 bytes
		{
			uint32_t val = 0; 
			memcpy(&val, data, 4);
			val = ntohl(val); //length
			if (val == count - 4 || count == 4) {
				iState = 3;
				return true;
			}
		}
	}
	if (iState == 2 && count == 12)  // 12 bytes
	{
		int procid = GetProcessId(NULL);
		memcpy(data, &procid, 4);
		DEBUGTRACELINE("Send back " << count << " bytes");
		if (send(sock, data, count, 0) == SOCKET_ERROR) {
			DEBUGTRACELINE("Send error " << WSAGetLastError());
			return false;
		}
		iState = 3;
		return true;
	}
	return false;
}


bool socketServer::streamPipe(SOCKET sock, sshMessage msg)
{
	if (!initPipe())
		return false; //upstream fail
	const int interval = 40;
	DWORD totalavail = 0, totalread = 0;

	DEBUGTRACELINE("Write to pipe>>>" <<  msg.header.size() << " + " << msg.content.size() << " bytes");
	if (!WriteFile(sc.pipe, msg.header.data(), (DWORD)msg.header.size(), NULL, NULL) ||
		!WriteFile(sc.pipe, msg.content.data(), (DWORD)msg.content.size(), NULL, NULL))
	{
		DEBUGTRACELINE("Write to pipe error data " << GetLastError());
		return false;
	}
	int sleeptime = 0;
	while (1)
	{
		sleeptime += interval;
		Sleep(interval);
		PeekNamedPipe(sc.pipe, NULL, NULL, NULL, &totalavail, NULL);
		DEBUGTRACELINE("total available bytes " << totalavail << "...wait..." << sleeptime << " ms");
		if (totalavail > 0)
			break;
		if (sleeptime > 60000) //60s timeout, start over again.
			return false; 
	}
	char buff[BUFLEN] = { 0 }; 
	while (1) {
		if (!ReadFile(sc.pipe, buff, BUFLEN, &totalread, NULL))
		{
			DEBUGTRACELINE("Get pipe content failed " << GetLastError());
			return false;
		}
		DEBUGTRACELINE("<<<Read from pipe and send back<<<" << totalread << " bytes");
		if (send(sock, buff, totalread, 0) == SOCKET_ERROR) {
			DEBUGTRACELINE("Send error " << WSAGetLastError());
			return false;
		}
		if (totalread < BUFLEN)
			break;
		PeekNamedPipe(sc.pipe, NULL, NULL, NULL, &totalavail, NULL);
		if (totalavail == 0)
			break;
	}
	return true;
}
