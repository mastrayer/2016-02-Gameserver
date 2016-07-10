#pragma once
#include <time.h>
#include <chrono>
#include <vector>
#include "Network.h"

struct ConnectedUser
{
	void Reset()
	{
		mLoginSuccess = false;
		mConnectedAt = 0;
	}

	bool mLoginSuccess = false;
	time_t mConnectedAt = 0;
};

class ConnectedUserManager
{
public:
	ConnectedUserManager() {}
	virtual ~ConnectedUserManager() {}

	void Init(Network *network);
	void ConnectSession(const int sessionID);
	void Login(const int sessionID);
	void DisconnectSession(const int sessionID);
	void LoginCheck();

private:
	Network* mNetwork;

	std::vector<ConnectedUser> ConnectedUserList;

	std::chrono::system_clock::time_point mLatestCheckTime = std::chrono::system_clock::now();
	int mLatestIndex = -1;
};