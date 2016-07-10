#include "PacketHandler.h"



PacketHandler::PacketHandler()
{
}


PacketHandler::~PacketHandler()
{
}

void PacketHandler::Init(Network *network, UserManager *UserManager)
{
	mNetwork = network;
	mUserManager = UserManager;

	mConnectedUserManager = std::make_unique<ConnectedUserManager>();
	mConnectedUserManager->Init(mNetwork);

	mHandler[(int)PACKET_ID::NTF_SYS_CONNECT_SESSION] = std::bind(&PacketHandler::ConnectSession, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::NTF_SYS_CLOSE_SESSION] = std::bind(&PacketHandler::CloseSession, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOGIN_IN_REQ]			= std::bind(&PacketHandler::Login, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOBBY_LIST_REQ]		= std::bind(&PacketHandler::LobbyList, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOBBY_ENTER_REQ]		= std::bind(&PacketHandler::LobbyEnter, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOBBY_LEAVE_REQ]		= std::bind(&PacketHandler::LobbyLeave, this, std::placeholders::_1);
}

void PacketHandler::Handle(RecvPacket & packet)
{
	// 해당 패킷 ID에 해당하는 핸들러가 없음
	if (mHandler[packet.PacketId] == nullptr)
		return;

	mHandler[packet.PacketId](packet);
}

ERROR_CODE PacketHandler::ConnectSession(RecvPacket & packet)
{
	mConnectedUserManager->ConnectSession(packet.SessionIndex);
	return ERROR_CODE();
}
ERROR_CODE PacketHandler::CloseSession(RecvPacket & packet)
{
	auto user = std::get<1>(mUserManager->GetUser(packet.SessionIndex));
	/*
	if (user)
	{
		auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
		if (pLobby)
		{
			auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());

			if (pRoom)
			{
				pRoom->LeaveUser(pUser->GetIndex());
				pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());
				pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

				m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Room Out", __FUNCTION__, packetInfo.SessionIndex);
			}

			pLobby->LeaveUser(pUser->GetIndex());

			if (pRoom == nullptr) {
				pLobby->NotifyLobbyLeaveUserInfo(pUser);
			}

			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Lobby Out", __FUNCTION__, packetInfo.SessionIndex);
		}

		m_pRefUserMgr->RemoveUser(packetInfo.SessionIndex);
	}

	m_pConnectedUserManager->SetDisConnectSession(packetInfo.SessionIndex);

	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d)", __FUNCTION__, packetInfo.SessionIndex);
	*/
	return ERROR_CODE::NONE;
}

ERROR_CODE PacketHandler::Login(RecvPacket & packet)
{
	CHECK_START
		PACKET::LogInResponse response;
		auto request = (PACKET::LogInRequest *)packet.Data;

		response.SetError(mUserManager->AddUser(packet.SessionIndex, request->ID));

		if (response.ErrorCode != (short)ERROR_CODE::NONE)
			CHECK_ERROR((ERROR_CODE)response.ErrorCode);

		mConnectedUserManager->Login(packet.SessionIndex);

		auto session = mNetwork->GetSession(packet.SessionIndex);
		session.
		


		mNetwork->SendData(packet.SessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(PACKET::LogInResponse), (char*)&response);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		response.SetError(__result);
		mNetwork->SendData(packet.SessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(PACKET::LogInResponse), (char*)&response);
		return (ERROR_CODE)__result;
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
