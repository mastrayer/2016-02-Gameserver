#include "Server.h"
#include "Network.h"
#include "LobbyManager.h"
#include <iostream>


Server::Server()
{
}


Server::~Server()
{
}

bool Server::Init()
{
	mNetwork = std::make_unique<Network>();
	auto result = mNetwork->Init();

	if (result != SERVER_ERROR::NONE) {
		std::cout << "SERVER ERROR! (" << (short)result << ")" << std::endl;
		return false;
	}

	mUserManager = std::make_unique<UserManager>();
	mUserManager->Init();

	m_pLobbyMgr = std::make_unique<LobbyManager>();
	m_pLobbyMgr->Init({ SERVER_CONFIG::MaxLobbyCount,
		SERVER_CONFIG::MaxLobbyUserCount,
		SERVER_CONFIG::MaxRoomCountByLobby,
		SERVER_CONFIG::MaxRoomUserCount },
		mNetwork.get());

	mPacketHandler = std::make_unique<PacketHandler>();
	mPacketHandler->Init(mNetwork.get(), mUserManager.get(), m_pLobbyMgr.get());

	mIsRunning = true;
	return true;
}

void Server::Run()
{
	while (mIsRunning) {
		mNetwork->Run();

		while (true)
		{
			auto packet = mNetwork->GetPacket();

			// 패킷 쌓인게 없다
			if (packet.PacketId == 0)
				break;

			mPacketHandler->Handle(packet);
		}

		mPacketHandler->StateCheck();
	}
}

void Server::Pause()
{
	mIsRunning = false;
}
