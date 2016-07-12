#include "PacketHandler.h"
#include "ConnectedUserManager.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "User.h"
#include "Lobby.h"


PacketHandler::PacketHandler()
{
}


PacketHandler::~PacketHandler()
{
}

void PacketHandler::Init(Network *network, UserManager *UserManager, LobbyManager* pLobbyMgr)
{
	mNetwork = network;
	mUserManager = UserManager;
	m_pRefLobbyMgr = pLobbyMgr;

	mConnectedUserManager = std::make_unique<ConnectedUserManager>();
	mConnectedUserManager->Init(mNetwork);

	mHandler[(int)PACKET_ID::NTF_SYS_CONNECT_SESSION]	= std::bind(&PacketHandler::ConnectSession, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::NTF_SYS_CLOSE_SESSION]		= std::bind(&PacketHandler::CloseSession, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOGIN_IN_REQ]				= std::bind(&PacketHandler::Login, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOBBY_LIST_REQ]			= std::bind(&PacketHandler::LobbyList, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOBBY_ENTER_REQ]			= std::bind(&PacketHandler::LobbyEnter, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOBBY_LEAVE_REQ]			= std::bind(&PacketHandler::LobbyLeave, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOBBY_ENTER_ROOM_LIST_REQ] = std::bind(&PacketHandler::LobbyRoomList, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::LOBBY_ENTER_USER_LIST_REQ] = std::bind(&PacketHandler::LobbyUserList, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::ROOM_ENTER_REQ]			= std::bind(&PacketHandler::RoomEnter, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::ROOM_LEAVE_REQ]			= std::bind(&PacketHandler::RoomLeave, this, std::placeholders::_1);
	mHandler[(int)PACKET_ID::ROOM_CHAT_REQ]				= std::bind(&PacketHandler::RoomChat, this, std::placeholders::_1);
}

void PacketHandler::Handle(RecvPacket & packet)
{
	// �ش� ��Ŷ ID�� �ش��ϴ� �ڵ鷯�� ����
	if (mHandler[packet.PacketId] == nullptr)
		return;

	mHandler[packet.PacketId](packet);
}
void PacketHandler::StateCheck()
{
	mConnectedUserManager->LoginCheck();
}

// ���� ȣ��Ǵ��� �𸣰ڴ�
ERROR_CODE PacketHandler::ConnectSession(RecvPacket & packet)
{
	mConnectedUserManager->ConnectSession(packet.SessionIndex);
	return ERROR_CODE();
}
ERROR_CODE PacketHandler::CloseSession(RecvPacket & packet)
{
	auto pUser = std::get<1>(mUserManager->GetUser(packet.SessionIndex));

	// �α��� �� ������ Ŀ�ؼ��� ���� ���
	if (pUser)
	{
		// ������ �������ִ� �κ� ������
		auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
		
		// �κ� ������ �־��ٸ�?
		if (pLobby)
		{
			// ������ �������ִ� ���� ������
			auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());

			// �뿡 ������ �־��ٸ�?
			if (pRoom)
			{
				// ������ �뿡�� �������� ���� �ٸ� �����鿡�� ��ε�ĳ��Ʈ
				pRoom->LeaveUser(pUser->GetIndex());
				pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());
				pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

				std::cout << __FUNCTION__ << " | NtfSysCloseSesson. sessionIndex(" << packet.SessionIndex << "). Room Out" << std::endl;
			}

			// �κ񿡼� ������ ������
			pLobby->LeaveUser(pUser->GetIndex());

			// �뿡 �������� �ʰ� �κ� �־��ٸ� �κ��� �ٸ� �����鿡�� ������ ��ε�ĳ��Ʈ
			if (pRoom == nullptr) {
				pLobby->NotifyLobbyLeaveUserInfo(pUser);
			}

			std::cout << __FUNCTION__ << " | NtfSysCloseSesson. sessionIndex(" << packet.SessionIndex << "). Lobby Out" << std::endl;
		}

		mUserManager->RemoveUser(packet.SessionIndex);
	}

	mConnectedUserManager->DisconnectSession(packet.SessionIndex);

	std::cout << __FUNCTION__ << " | NtfSysCloseSesson. sessionIndex(" << packet.SessionIndex << ")" << std::endl;
	return ERROR_CODE::NONE;
}

ERROR_CODE PacketHandler::Login(RecvPacket & packet)
{
	auto session = mNetwork->GetSession(packet.SessionIndex);
	PACKET::Header header{ (short)PACKET_ID::LOGIN_IN_RES, sizeof(PACKET::LogInResponse) };

	CHECK_START
		PACKET::LogInResponse response;
		auto request = (PACKET::LogInRequest *)packet.Data;

		response.SetError(mUserManager->AddUser(packet.SessionIndex, request->ID));

		if (response.ErrorCode != (short)ERROR_CODE::NONE) {
			CHECK_ERROR((ERROR_CODE)response.ErrorCode);
		}

		mConnectedUserManager->Login(packet.SessionIndex);
		mNetwork->AddSendQueue(session, header, (char*)&response);
		std::cout << "User Login [" << session.Index << " / " << request->ID << " / " << session.IP << "]" << std::endl;
		
		return ERROR_CODE::NONE;

	CHECK_ERR:
		response.SetError(__result);
		mNetwork->AddSendQueue(session, header, (char*)&response);
		return (ERROR_CODE)__result;
}

// �κ� ����Ʈ ��û
ERROR_CODE PacketHandler::LobbyList(RecvPacket & packet)
{
CHECK_START
	// ���� ���� �����ΰ�?
	// ���� �κ� ���� ���� �����ΰ�?

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// �α��� ���� ���� ������ �κ� ����Ʈ�� ��û�ϸ� ����
	if (pUser->InLogIn() == false) {
		CHECK_ERROR(ERROR_CODE::LOBBY_LIST_INVALID_DOMAIN);
	}

	m_pRefLobbyMgr->SendLobbyListInfo(packet.SessionIndex);

	return ERROR_CODE::NONE;

CHECK_ERR:
	PACKET::PktLobbyListRes resPkt;
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::LOBBY_LIST_RES, sizeof(resPkt) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// �κ� ���� ��û
ERROR_CODE PacketHandler::LobbyEnter(RecvPacket & packet)
{
CHECK_START
	// ���� ��ġ ���´� �α����� �³�?
	// �κ� ����.
	// ���� �κ� �ִ� ������� �� ����� ���Դٰ� �˷��ش�

	auto reqPkt = (PACKET::PktLobbyEnterReq*)packet.Data;
	PACKET::PktLobbyEnterRes resPkt;

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// �α������� ���� ������ �κ� �����û�ϸ� ����
	if (pUser->InLogIn() == false) {
		CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN);
	}

	// ���� ��û�� �κ� ��ȿ���� ���� �κ�� ����
	auto pLobby = m_pRefLobbyMgr->GetLobby(reqPkt->LobbyId);
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX);
	}

	// �κ� ����
	auto enterRet = pLobby->EnterUser(pUser);
	if (enterRet != ERROR_CODE::NONE) {
		CHECK_ERROR(enterRet);
	}

	// �κ� �������ִ� �ٸ� �����鿡�� �� ������ ������ ��ε�ĳ��Ʈ
	pLobby->NotifyLobbyEnterUserInfo(pUser);

	resPkt.MaxUserCount = pLobby->MaxUserCount();
	resPkt.MaxRoomCount = pLobby->MaxRoomCount();
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(PACKET::PktLobbyEnterRes) }, (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(PACKET::PktLobbyEnterRes) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// �κ� ������ ��û
ERROR_CODE PacketHandler::LobbyLeave(RecvPacket & packet)
{
CHECK_START
	// ���� �κ� �ִ��� �����Ѵ�.
	// �κ񿡼� ������
	// ���� �κ� �ִ� ������� ������ ����� �ִٰ� �˷��ش�.
	PACKET::PktLobbyLeaveRes resPkt;

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// �κ� �������� ���� ������ ������ ��û�ϸ� ����
	if (pUser->InLobby() == false) {
		CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_DOMAIN);
	}

	// ��ȿ���� ���� �κ�� ����
	auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_LOBBY_INDEX);
	}

	// ���� ������ ó��
	auto enterRet = pLobby->LeaveUser(pUser->GetIndex());
	if (enterRet != ERROR_CODE::NONE) {
		CHECK_ERROR(enterRet);
	}

	// �κ��� �ٸ� �����鿡�� �� ������ ������ ��ε�ĳ��Ʈ
	pLobby->NotifyLobbyLeaveUserInfo(pUser);

	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(PACKET::PktLobbyLeaveRes) }, (char*)&resPkt);

	return ERROR_CODE::NONE;
CHECK_ERR:
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{(short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(PACKET::PktLobbyLeaveRes)}, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// �κ��� �� ����Ʈ ��û
ERROR_CODE PacketHandler::LobbyRoomList(RecvPacket & packet)
{
CHECK_START
	// ���� �κ� �ִ��� �����Ѵ�.
	// �� ����Ʈ�� �����ش�.

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// �κ� ���� ���� ������ �κ��� �� ����Ʈ ��û�ϸ� ����
	if (pUser->InLobby() == false) {
		CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_DOMAIN);
	}

	// ��û�� �κ� ��ȿ���� ���� �κ�� ����
	auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_LOBBY_INDEX);
	}

	auto reqPkt = (PACKET::PktLobbyRoomListReq*)packet.Data;

	pLobby->SendRoomList(pUser->GetSessioID(), reqPkt->StartRoomIndex);

	return ERROR_CODE::NONE;
CHECK_ERR:
	PACKET::PktLobbyRoomListRes resPkt;
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(PACKET::Base) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// �κ� ������ ���� ����Ʈ ��û
ERROR_CODE PacketHandler::LobbyUserList(RecvPacket & packet)
{
CHECK_START
	// ���� �κ� �ִ��� �����Ѵ�.
	// ���� ����Ʈ�� �����ش�.

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// �κ� �������� ���� ������ ���� ����Ʈ ��û�ϸ� ����
	if (pUser->InLobby() == false) {
		CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
	}

	// �κ� ��ȿ���� ������ ����
	auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_LOBBY_INDEX);
	}

	auto reqPkt = (PACKET::PktLobbyUserListReq*)packet.Data;

	pLobby->SendUserList(pUser->GetSessioID(), reqPkt->StartUserIndex);

	return ERROR_CODE::NONE;
CHECK_ERR:
	PACKET::PktLobbyUserListRes resPkt;
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(PACKET::Base) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// �뿡 ���� ��û
ERROR_CODE PacketHandler::RoomEnter(RecvPacket & packet)
{
CHECK_START
	auto reqPkt = (PACKET::PktRoomEnterReq*)packet.Data;
	PACKET::PktRoomEnterRes resPkt;

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// �κ� �������� ���� ������ �뿡 �����Ϸ��� �ϸ� ����
	if (pUser->InLobby() == false) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	// ��ȿ���� ���� �κ�� ����
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
	}

	// ��ȿ���� ���� ���̸� ����
	auto pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
	if (pRoom == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
	}

	// ���� ����� ����� ���� �����
	if (reqPkt->IsCreate)
	{
		auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
		if (ret != ERROR_CODE::NONE) {
			CHECK_ERROR(ret);
		}
	}

	// �뿡 ������ �����Ŵ
	auto enterRet = pRoom->EnterUser(pUser);
	if (enterRet != ERROR_CODE::NONE) {
		CHECK_ERROR(enterRet);
	}

	// ���� ������ �뿡 ���Դٰ� �����Ѵ�.
	pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

	// �κ� ������ �������� �˸���
	pLobby->NotifyLobbyLeaveUserInfo(pUser);

	// �κ� �� ������ �뺸�Ѵ�.
	pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

	// �뿡 �� ���� ���Դٰ� �˸���
	pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());

	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt) }, (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// �� ������ ��û
ERROR_CODE PacketHandler::RoomLeave(RecvPacket & packet)
{
CHECK_START
	PACKET::PktRoomLeaveRes resPkt;

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);
	auto userIndex = pUser->GetIndex();

	// �뿡 �������� ���� ������ �� ������ ��û�ϸ� ����
	if (pUser->InRoom() == false) {
		CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	// �߸��� �κ�� ����
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
	}

	// ��ȿ���� ���� ���̸� ����
	auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
	if (pRoom == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
	}

	auto leaveRet = pRoom->LeaveUser(userIndex);
	if (leaveRet != ERROR_CODE::NONE) {
		CHECK_ERROR(leaveRet);
	}

	// ���� ������ �κ�� ����
	pUser->EnterLobby(lobbyIndex);

	// �뿡 ������ �������� �뺸
	pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

	// �κ� ���ο� ������ �������� �뺸
	pLobby->NotifyLobbyEnterUserInfo(pUser);

	// �κ� �ٲ� �� ������ �뺸
	pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt) }, (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// �� ä�� ��û
ERROR_CODE PacketHandler::RoomChat(RecvPacket & packet)
{
CHECK_START
	auto reqPkt = (PACKET::PktRoomChatReq*)packet.Data;
	PACKET::PktRoomChatRes resPkt;

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// �뿡 �������� ���� ������ ä��ġ���� �ϸ� ����
	if (pUser->InRoom() == false) {
		CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	// ��ȿ���� ���� �κ�� ����
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
	}

	auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
	// ��ȿ���� ���� ���̸� ����
	if (pRoom == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
	}

	// �뿡 ������ �ٸ� �����鿡�� ä�ø޼��� ��ε�ĳ��Ʈ
	pRoom->NotifyChat(pUser->GetSessioID(), pUser->GetID().c_str(), reqPkt->Msg);

	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt) }, (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}
