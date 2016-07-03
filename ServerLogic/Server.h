#pragma once
#include <memory>

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
	std::unique_ptr<Network> mNetwork = nullptr;
};

