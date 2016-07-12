#pragma once
#include <unordered_map>
#include <functional>
#include <memory>
#include "Network.h"


#define CHECK_START  ERROR_CODE __result=ERROR_CODE::NONE;
#define CHECK_ERROR(f) __result=f; goto CHECK_ERR;

class Network;
class UserManager;
class LobbyManager;
class ConnectedUserManager;

class PacketHandler
{
public:
	PacketHandler();
	~PacketHandler();

	void Init(Network *network, UserManager *UserManager, LobbyManager* pLobbyMgr);
	void Handle(RecvPacket &packet);
	void StateCheck();

private:
	std::unordered_map<int, std::function<ERROR_CODE (RecvPacket &)>> mHandler;
	std::unique_ptr<ConnectedUserManager> mConnectedUserManager;
	Network *mNetwork;
	UserManager *mUserManager;
	LobbyManager* m_pRefLobbyMgr;

	ERROR_CODE ConnectSession(RecvPacket &packet);
	ERROR_CODE CloseSession(RecvPacket &packet);
	ERROR_CODE Login(RecvPacket &packet);
	ERROR_CODE LobbyList(RecvPacket &packet);
	ERROR_CODE LobbyEnter(RecvPacket &packet);
	ERROR_CODE LobbyLeave(RecvPacket &packet);
	ERROR_CODE LobbyRoomList(RecvPacket &packet);
	ERROR_CODE LobbyUserList(RecvPacket &packet);
	ERROR_CODE RoomEnter(RecvPacket &packet);
	ERROR_CODE RoomLeave(RecvPacket &packet);
	ERROR_CODE RoomChat(RecvPacket &packet);
};

