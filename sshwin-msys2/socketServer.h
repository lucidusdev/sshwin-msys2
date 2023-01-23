#pragma once

#include <string>
#include <winSock2.h>
#include <Windows.h>

#define BUFLEN  4096

class socketServer
{
	public:
		struct serverConfig {
			std::string socketFile;
			std::string IP;
			std::string port;
			std::string pipeName = "openssh-ssh-agent";
			int timeout = 60000;  //in ms.

			std::string guid = "";
			HANDLE pipe = INVALID_HANDLE_VALUE;
		};
		socketServer(serverConfig sc);
		virtual ~socketServer();
		void Start();

	private:
		struct sshMessage {
			uint32_t length = 0;
			std::vector<char> header;
			std::vector<char> content;
			void reset() {
				length = 0;
				header.clear();
				content.clear();
			}
		};
		serverConfig sc;
		std::string initMsys2Socket();
		int proxyServer();
		bool initConnection(SOCKET sClient, char* buff, int count, int& iState);
		bool initPipe();
		bool getPipename(std::string & pname);		
		bool streamPipe(SOCKET sock, sshMessage msg);


};

