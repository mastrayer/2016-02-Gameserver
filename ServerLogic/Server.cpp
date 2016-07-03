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

	mIsRunning = true;
	return true;
}

void Server::Run()
{
	while (mIsRunning) {
		mNetwork->Run();

		while (false) {

		}
	}
}

void Server::Pause()
{
	mIsRunning = false;
}
