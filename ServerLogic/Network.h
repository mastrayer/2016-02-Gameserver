#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <deque>
#include <iostream>
#include "ServerCode.h"
#include "Packet.h"
#include "PacketID.h"
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
struct RecvPacket
{
	int SessionIndex = 0;
	short PacketId = 0;
	short PacketBodySize = 0;
	char* Data = 0;
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
	void		 ProcessRequest(fd_set &exc_set, fd_set &read_set, fd_set &write_set);
	bool		 RecvFromSocket(Session &session, fd_set &read_set);
	void		 SendData(Session &session, fd_set &write_set);

	void		 AddPacket(const Session &session, const PACKET::Header &header, char* data);

	std::vector<Session>	mSessionPool;
	std::deque<int>			mAvailablePoolIndex;
	std::deque<RecvPacket>	mPackets;

	int mConnectedSessions = 0;

	fd_set mReadFDSet;
	SOCKET mServerSocket;
};

