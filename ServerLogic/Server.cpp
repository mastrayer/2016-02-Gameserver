#include "Server.h"
#include "Network.h"
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

	mPacketHandler = std::make_unique<PacketHandler>();
	mPacketHandler->Init();

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

		std::this_thread::sleep_for(std::chrono::milliseconds(0));
	}
}

void Server::Pause()
{
	mIsRunning = false;
}
