#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <deque>
#include <iostream>
#include "ServerCode.h"
#include "Config.h"

struct Session
{
	bool IsConnected() { return SocketFD > 0; }

	void Clear()
	{
		Seq = 0;
		SocketFD = 0;
		IP[0] = NULL;
		RemainingDataSize = 0;
		SendSize = 0;
	}

	int Index = 0;
	long long Seq = 0;
	int		SocketFD = 0;
	char    IP[SERVER_CONFIG::ip_length] = { 0, };

	char   *RecvBuffer = nullptr;
	int     RemainingDataSize = 0;

	char   *SendBuffer = nullptr;
	int     SendSize = 0;
};

class Network
{
public:
	Network();
	~Network();

	void Run();
	SERVER_ERROR Init();
	
private:
	SERVER_ERROR AllocateSession();
	void		 CloseSession(const SOCKET_CLOSE CloseCase, const SOCKET socketFD, const int sessionId);
	void		 

	std::vector<Session> mSessionPool;
	std::deque<int>	 mAvailablePoolIndex;

	int mConnectedSessions = 0;

	fd_set mReadFDSet;
	SOCKET mServerSocket;
};

