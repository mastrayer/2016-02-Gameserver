#pragma once
#include <vector>
#include <unordered_map>
#include "Network.h"

struct LobbyManagerConfig
{
	int MaxLobbyCount;
	int MaxLobbyUserCount;
	int MaxRoomCountByLobby;
	int MaxRoomUserCount;
};

struct LobbySmallInfo
{
	short Num;
	short UserCount;
};

class Lobby;

class LobbyManager
{
public:
	LobbyManager();
	virtual ~LobbyManager();

	void Init(const LobbyManagerConfig config, Network* pNetwork);

	Lobby* GetLobby(short lobbyId);


public:
	void SendLobbyListInfo(const int sessionIndex);





private:
	Network* m_pRefNetwork;

	std::vector<Lobby> m_LobbyList;

};