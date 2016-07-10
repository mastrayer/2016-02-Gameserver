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

	// 60밀리초 마다 검사
	if (duration.count() < 60)
		return;
	else
		mLatestCheckTime = now;

	auto current_second = std::chrono::system_clock::to_time_t(now);

	if (mLatestIndex >= SERVER_CONFIG::max_connection)
		mLatestIndex = -1;

	++mLatestIndex;


	// 뭐하는 구문이지 이게???? -----------------------
	auto lastCheckIndex = mLatestIndex + 100;
	if (lastCheckIndex > SERVER_CONFIG::max_connection)
		lastCheckIndex = SERVER_CONFIG::max_connection;
	// 뭐하는 구문이지 이게???? -----------------------


	for (; mLatestIndex < lastCheckIndex; ++mLatestIndex)
	{
		// connetedAt == 0은 왜 그런지 모르겠고 로그인 성공한 유저면 로그인 체크를 하지 않는다
		if (ConnectedUserList[mLatestIndex].mConnectedAt == 0 || ConnectedUserList[mLatestIndex].mLoginSuccess)
			continue;

		// 3초? 3분동안 커넥션만 맺고 로그인 안하면 강제로 소켓 닫기
		auto diff = current_second - ConnectedUserList[mLatestIndex].mConnectedAt;
		if (diff >= 3)
			mNetwork->ForcingClose(mLatestIndex);
	}
}