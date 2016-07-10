#include "ConnectedUserManager.h"



ConnectedUserManager::ConnectedUserManager()
{
}


ConnectedUserManager::~ConnectedUserManager()
{
}
void ConnectedUserManager::Init(Network *network)
{
	mNetwork = network;

	for (int i = 0; i < SERVER_CONFIG::max_connection; ++i)
		ConnectedUserList.emplace_back(ConnectedUser());
}

void ConnectedUserManager::ConnectSession(const int sessionID)
{
	time(&ConnectedUserList[sessionID].mConnectedAt);
}

void ConnectedUserManager::Login(const int sessionID)
{
	ConnectedUserList[sessionID].mLoginSuccess = true;
}

void ConnectedUserManager::DisconnectSession(const int sessionID)
{
	ConnectedUserList[sessionID].Reset();
}

void ConnectedUserManager::LoginCheck()
{
	auto now = std::chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - mLatestCheckTime);

	// 60�и��� ���� �˻�
	if (duration.count() < 60)
		return;
	else
		mLatestCheckTime = now;

	auto current_second = std::chrono::system_clock::to_time_t(now);

	if (mLatestIndex >= SERVER_CONFIG::max_connection)
		mLatestIndex = -1;

	++mLatestIndex;


	// ���ϴ� �������� �̰�???? -----------------------
	auto lastCheckIndex = mLatestIndex + 100;
	if (lastCheckIndex > SERVER_CONFIG::max_connection)
		lastCheckIndex = SERVER_CONFIG::max_connection;
	// ���ϴ� �������� �̰�???? -----------------------


	for (; mLatestIndex < lastCheckIndex; ++mLatestIndex)
	{
		// connetedAt == 0�� �� �׷��� �𸣰ڰ� �α��� ������ ������ �α��� üũ�� ���� �ʴ´�
		if (ConnectedUserList[mLatestIndex].mConnectedAt == 0 || ConnectedUserList[mLatestIndex].mLoginSuccess)
			continue;

		// 3��? 3�е��� Ŀ�ؼǸ� �ΰ� �α��� ���ϸ� ������ ���� �ݱ�
		auto diff = current_second - ConnectedUserList[mLatestIndex].mConnectedAt;
		if (diff >= 3)
			mNetwork->ForcingClose(mLatestIndex);
	}
}