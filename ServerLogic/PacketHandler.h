#pragma once
#include <unordered_map>
#include <functional>
#include "Network.h"

class PacketHandler
{
public:
	PacketHandler();
	~PacketHandler();

	void Init();
	void Handle(RecvPacket &packet);

private:
	std::unordered_map<int, std::function<void(RecvPacket &)>> mHandler;


	ERROR_CODE CloseSession(RecvPacket &packet);
	ERROR_CODE Login(RecvPacket &packet);
	ERROR_CODE LobbyList(RecvPacket &packet);
	ERROR_CODE LobbyEnter(RecvPacket &packet);
	ERROR_CODE LobbyLeave(RecvPacket &packet);
};

