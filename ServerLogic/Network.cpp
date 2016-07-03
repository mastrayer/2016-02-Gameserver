#include "Network.h"



Network::Network()
{
}


Network::~Network()
{
}

void Network::Run()
{
	auto read_set = mReadFDSet;
	auto write_set = mReadFDSet;
	auto exc_set = mReadFDSet;

	timeval timeout{ 0, 1000 }; //tv_sec, tv_usec
	auto result = select(0, &read_set, &write_set, &exc_set, &timeout);

	// no selected fd
	if (result == 0 && result == -1)
		return;

	// accept
	if (FD_ISSET(mServerSocket, &read_set))
		AllocateSession();
	else // clients
		RunCheckSelectClients(exc_set, read_set, write_set);
}

SERVER_ERROR Network::Init()
{
	// Create Socket
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	mServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mServerSocket < 0)
		return SERVER_ERROR::SERVER_SOCKET_CREATE_FAIL;

	auto opt = 1;
	if (setsockopt(mServerSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
		return SERVER_ERROR::SERVER_SOCKET_SO_REUSEADDR_FAIL;


	// Bind
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVER_CONFIG::port);

	if (bind(mServerSocket, (SOCKADDR*)&server_addr, sizeof(server_addr)) < 0)
		return SERVER_ERROR::SERVER_SOCKET_BIND_FAIL;

	// Listen
	if (listen(mServerSocket, SERVER_CONFIG::backlog_size) == SOCKET_ERROR)
		return SERVER_ERROR::SERVER_SOCKET_LISTEN_FAIL;

	return SERVER_ERROR::NONE;
}
SERVER_ERROR Network::AllocateSession()
{
	// accept
	SOCKADDR_IN client_addr;
	auto client_len = static_cast<int>(sizeof(client_addr));
	auto client_sockfd = accept(mServerSocket, (SOCKADDR*)&client_addr, &client_len);

	if (client_sockfd < 0)
	{
		std::cout << __FUNCTION__ << " : Wrong Socket " << client_sockfd << std::endl;
		return SERVER_ERROR::ACCEPT_API_ERROR;
	}


	// allocate session id
	if (mAvailablePoolIndex.empty())
	{
		// connection overflow
		std::cout << __FUNCTION__ << " : Maximum session" << std::endl;

		CloseSession(SOCKET_CLOSE::SESSION_POOL_EMPTY, client_sockfd, -1);
		return SERVER_ERROR::ACCEPT_MAX_SESSION_COUNT;
	}
	int session_id = mAvailablePoolIndex.front();
	mAvailablePoolIndex.pop_front();


	// socket setting
	char clientIP[SERVER_CONFIG::ip_length] = { 0, };
	inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, SERVER_CONFIG::ip_length - 1);

	linger ling;
	ling.l_onoff = 0;
	ling.l_linger = 0;
	setsockopt(client_sockfd, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

	int size1 = SERVER_CONFIG::MaxClientSockOptRecvBufferSize;
	int size2 = SERVER_CONFIG::MaxClientSockOptSendBufferSize;
	setsockopt(client_sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&size1, sizeof(size1));
	setsockopt(client_sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&size2, sizeof(size2));

	FD_SET(client_sockfd, &mReadFDSet);


	// session init
	++mConnectedSessions;

	auto& session = mSessionPool[session_id];
	session.Seq = mConnectedSessions;
	session.SocketFD = client_sockfd;
	memcpy(session.IP, clientIP, SERVER_CONFIG::ip_length - 1);

	std::cout << __FUNCTION__ << " : New Session " << clientIP << std::endl;

	return SERVER_ERROR::NONE;
}
void Network::CloseSession(const SOCKET_CLOSE CloseCase, const SOCKET socketFD, const int sessionId)
{
	if (CloseCase == SOCKET_CLOSE::SESSION_POOL_EMPTY)
	{
		closesocket(socketFD);
		FD_CLR(socketFD, &mReadFDSet);

		return;
	}

	if (!mSessionPool[sessionId].IsConnected())
		return;

	closesocket(socketFD);
	FD_CLR(socketFD, &mReadFDSet);

	--mConnectedSessions;

	mAvailablePoolIndex.push_back(sessionId);
	mSessionPool[sessionId].Clear();

	//AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
}
