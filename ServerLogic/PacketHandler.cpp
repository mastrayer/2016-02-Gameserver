#include "PacketHandler.h"



PacketHandler::PacketHandler()
{
}


PacketHandler::~PacketHandler()
{
}

void PacketHandler::Init()
{
	mHandler[(int)PACKET_ID::NTF_SYS_CLOSE_SESSION] = &PacketHandler::CloseSession;
	mHandler[(int)PACKET_ID::LOGIN_IN_REQ]			= &PacketHandler::Login;
	mHandler[(int)PACKET_ID::LOBBY_LIST_REQ]		= &PacketHandler::LobbyList;
	mHandler[(int)PACKET_ID::LOBBY_ENTER_REQ]		= &PacketHandler::LobbyEnter;
	mHandler[(int)PACKET_ID::LOBBY_LEAVE_REQ]		= &PacketHandler::LobbyLeave;
}

void PacketHandler::Handle(RecvPacket & packet)
{
	// 해당 패킷 ID에 해당하는 핸들러가 없음
	if (mHandler[packet.PacketId] == nullptr)
		return;

	mHandler[packet.PacketId](packet);
}

ERROR_CODE PacketHandler::CloseSession(RecvPacket & packet)
{
	return ERROR_CODE();
}

ERROR_CODE PacketHandler::Login(RecvPacket & packet)
{
	return ERROR_CODE();
}

ERROR_CODE PacketHandler::LobbyList(RecvPacket & packet)
{
	return ERROR_CODE();
}

ERROR_CODE PacketHandler::LobbyEnter(RecvPacket & packet)
{
	return ERROR_CODE();
}

ERROR_CODE PacketHandler::LobbyLeave(RecvPacket & packet)
{
	return ERROR_CODE();
}
