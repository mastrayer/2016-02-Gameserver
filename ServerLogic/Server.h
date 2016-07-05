#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include <thread>
#include <chrono>
#include "PacketHandler.h"

class Network;

class Server
{
public:
	Server();
	~Server();

	bool Init();
	void Run();
	void Pause();

private:
	bool mIsRunning = false;

	std::unique_ptr<Network>		mNetwork;
	std::unique_ptr<PacketHandler>	mPacketHandler;
};

