#include "Network.h"



Network::Network()
{
}


Network::~Network()
{
	for (auto& session : mSessionPool)
	{
		if (session.RecvBuffer)
			delete[] session.RecvBuffer;

		if (session.SendBuffer)
			delete[] session.SendBuffer;
	}
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
	else // check request
		ProcessRequest(exc_set, read_set, write_set);
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
	
	// Create Session Pool
	for (int i = 0; i < SERVER_CONFIG::max_connection; ++i)
	{
		Session session;
		ZeroMemory(&session, sizeof(session));

		session.Index = i;
		session.RecvBuffer = new char[SERVER_CONFIG::MaxClientRecvBufferSize];
		session.SendBuffer = new char[SERVER_CONFIG::MaxClientSendBufferSize];

		mSessionPool.push_back(session);
		mAvailablePoolIndex.push_back(session.Index);
	}

	FD_ZERO(&mReadFDSet);
	FD_SET(mServerSocket, &mReadFDSet);

	return SERVER_ERROR::NONE;
}
SERVER_ERROR Network::AddSendQueue(Session &session, const PACKET::Header &header, const char *data)
{
	auto pos = session.SendSize;

	if ((pos + header.BodySize + sizeof(PACKET::Header)) > SERVER_CONFIG::MaxClientSendBufferSize) {
		return SERVER_ERROR::CLIENT_SEND_BUFFER_FULL;
	}

	memcpy(&session.SendBuffer[pos], (char*)&header, sizeof(PACKET::Header));
	memcpy(&session.SendBuffer[pos + sizeof(PACKET::Header)], data, header.BodySize);
	session.SendSize += (header.BodySize + sizeof(PACKET::Header));

	return SERVER_ERROR::NONE;
}
RecvPacket Network::GetPacket()
{
	RecvPacket packet;

	if (mPackets.empty() == false)
	{
		packet = mPackets.front();
		mPackets.pop_front();
	}

	return packet;
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

	PACKET::Header header = { (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0 };
	AddPacket(mSessionPool[sessionId], header, nullptr);
}

void Network::ProcessRequest(fd_set &exc_set, fd_set &read_set, fd_set &write_set)
{
	for (auto &session : mSessionPool) {
		// 접속된 세션만 체크함
		if (!session.IsConnected())
			continue;

		if (FD_ISSET(session.SocketFD, &exc_set)) {
			CloseSession(SOCKET_CLOSE::SELECT_ERROR, session.SocketFD, session.Index);
			continue;
		}

		// read
		if (!RecvFromSocket(session, read_set))
			continue;

		// write
		SendData(session, write_set);
	}
}
bool Network::RecvFromSocket(Session &session, fd_set &read_set)
{
	// 읽을 데이터가 없다
	if (!FD_ISSET(session.SocketFD, &read_set))
		return true;

	if (session.IsConnected() == false) {
		CloseSession(SOCKET_CLOSE::SOCKET_RECV_ERROR, session.SocketFD, session.Index);
		return false;
	}

	int recvPos = session.RemainingDataSize;
	session.RemainingDataSize = 0;

	auto recvSize = recv(session.SocketFD, &session.RecvBuffer[recvPos], (SERVER_CONFIG::max_packet_size * 2), 0);

	if (recvSize == 0) {
		CloseSession(SOCKET_CLOSE::SOCKET_RECV_ERROR, session.SocketFD, session.Index);
		return false;
	} else if (recvSize < 0) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			CloseSession(SOCKET_CLOSE::SOCKET_RECV_ERROR, session.SocketFD, session.Index);
			return false;
		}
	}
	session.RemainingDataSize += recvSize;


	// 패킷 파싱
	auto readPos = 0;
	const auto dataSize = session.RemainingDataSize;
	PACKET::Header* header;

	while ((dataSize - readPos) >= sizeof(PACKET::Header)) {
		header = (PACKET::Header *)&session.RecvBuffer[readPos];
		readPos += sizeof(PACKET::Header);

		// 파싱할 데이터가 더 남아있는가?
		if (header->BodySize > 0) {
			// 버퍼에 저장된 데이터가 짤렸는가>
			if (header->BodySize < (dataSize - readPos))
				break;

			// 전송된 패킷이 최대 패킷 사이즈보다 큰가?
			if (header->BodySize > SERVER_CONFIG::max_packet_size) {
				CloseSession(SOCKET_CLOSE::SOCKET_RECV_BUFFER_PROCESS_ERROR, session.SocketFD, session.Index);
				return false;
			}
		}

		AddPacket(session, *header, &session.RecvBuffer[readPos]);

		readPos += header->BodySize;
	}

	session.RemainingDataSize -= readPos;

	// 처리하지 못한 데이터를 버퍼에 남겨두기
	if (session.RemainingDataSize > 0)
		memcpy(session.RecvBuffer, &session.RecvBuffer[readPos], session.RemainingDataSize);

	return true;
}
void Network::SendData(const int sessionID, const short packetID, const short size, const char * data)
{
}
void Network::SendData(Session &session, fd_set &write_set)
{
	// 쓸 데이터가 없다
	if (!FD_ISSET(session.SocketFD, &write_set))
		return;

	SendData(session);
}
void Network::SendData(Session &session)
{
	if (session.IsConnected() == false) {
		CloseSession(SOCKET_CLOSE::SOCKET_SEND_ERROR, session.SocketFD, session.Index);
		return;
	}

	int SendSize = 0;
	// 데이터 전송
	if (session.SendSize > 0) { // 보낼 데이터가 있는지
		SendSize = send(session.SocketFD, session.SendBuffer, session.SendSize, 0);

		if (SendSize <= 0) {
			CloseSession(SOCKET_CLOSE::SOCKET_SEND_ERROR, session.SocketFD, session.Index);
			return;
		}
	}

	// 데이터를 다 보내지 못함
	if (SendSize < session.SendSize)
	{
		memmove(&session.SendBuffer[0],
			&session.SendBuffer[SendSize],
			session.SendSize - SendSize);

		session.SendSize -= SendSize;
	}
	else
		session.SendSize = 0;
}
void Network::AddPacket(const Session &session, const PACKET::Header &header, char* data)
{
	RecvPacket packetInfo;
	packetInfo.SessionIndex = session.Index;
	packetInfo.PacketId = header.ID;
	packetInfo.PacketBodySize = header.BodySize;
	packetInfo.Data = data;

	mPackets.push_back(packetInfo);
}
void Network::ForcingClose(const int sessionID)
{
	if (!mSessionPool[sessionID].IsConnected())
		return;

	CloseSession(SOCKET_CLOSE::FORCING_CLOSE, mSessionPool[sessionID].SocketFD, sessionID);
}