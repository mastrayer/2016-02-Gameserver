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
	// 해당 패킷 ID에 해당하는 핸들러가 없음
	if (mHandler[packet.PacketId] == nullptr)
		return;

	mHandler[packet.PacketId](packet);
}
void PacketHandler::StateCheck()
{
	mConnectedUserManager->LoginCheck();
}

// 언제 호출되는지 모르겠다
ERROR_CODE PacketHandler::ConnectSession(RecvPacket & packet)
{
	mConnectedUserManager->ConnectSession(packet.SessionIndex);
	return ERROR_CODE();
}
ERROR_CODE PacketHandler::CloseSession(RecvPacket & packet)
{
	auto pUser = std::get<1>(mUserManager->GetUser(packet.SessionIndex));

	// 로그인 한 유저가 커넥션을 끊을 경우
	if (pUser)
	{
		// 유저가 접속해있던 로비를 가져옴
		auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
		
		// 로비에 접속해 있었다면?
		if (pLobby)
		{
			// 유저가 접속해있던 룸을 가져옴
			auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());

			// 룸에 접속해 있었다면?
			if (pRoom)
			{
				// 유저를 룸에서 내보내고 룸의 다른 유저들에게 브로드캐스트
				pRoom->LeaveUser(pUser->GetIndex());
				pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());
				pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

				std::cout << __FUNCTION__ << " | NtfSysCloseSesson. sessionIndex(" << packet.SessionIndex << "). Room Out" << std::endl;
			}

			// 로비에서 유저를 내보냄
			pLobby->LeaveUser(pUser->GetIndex());

			// 룸에 접속하지 않고 로비에 있었다면 로비의 다른 유저들에게 나간걸 브로드캐스트
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

// 로비 리스트 요청
ERROR_CODE PacketHandler::LobbyList(RecvPacket & packet)
{
CHECK_START
	// 인증 받은 유저인가?
	// 아직 로비에 들어가지 않은 유저인가?

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// 로그인 하지 않은 유저가 로비 리스트를 요청하면 에러
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

// 로비 입장 요청
ERROR_CODE PacketHandler::LobbyEnter(RecvPacket & packet)
{
CHECK_START
	// 현재 위치 상태는 로그인이 맞나?
	// 로비에 들어간다.
	// 기존 로비에 있는 사람에게 새 사람이 들어왔다고 알려준다

	auto reqPkt = (PACKET::PktLobbyEnterReq*)packet.Data;
	PACKET::PktLobbyEnterRes resPkt;

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// 로그인하지 않은 유저가 로비에 입장요청하면 에러
	if (pUser->InLogIn() == false) {
		CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN);
	}

	// 입장 요청한 로비가 유효하지 않은 로비면 에러
	auto pLobby = m_pRefLobbyMgr->GetLobby(reqPkt->LobbyId);
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX);
	}

	// 로비에 입장
	auto enterRet = pLobby->EnterUser(pUser);
	if (enterRet != ERROR_CODE::NONE) {
		CHECK_ERROR(enterRet);
	}

	// 로비에 접속해있던 다른 유저들에게 새 유저의 입장을 브로드캐스트
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

// 로비 나가기 요청
ERROR_CODE PacketHandler::LobbyLeave(RecvPacket & packet)
{
CHECK_START
	// 현재 로비에 있는지 조사한다.
	// 로비에서 나간다
	// 기존 로비에 있는 사람에게 나가는 사람이 있다고 알려준다.
	PACKET::PktLobbyLeaveRes resPkt;

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// 로비에 접속하지 않은 유저가 나가기 요청하면 에러
	if (pUser->InLobby() == false) {
		CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_DOMAIN);
	}

	// 유효하지 않은 로비면 에러
	auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_LOBBY_INDEX);
	}

	// 유저 나가기 처리
	auto enterRet = pLobby->LeaveUser(pUser->GetIndex());
	if (enterRet != ERROR_CODE::NONE) {
		CHECK_ERROR(enterRet);
	}

	// 로비의 다른 유저들에게 이 유저가 나간걸 브로드캐스트
	pLobby->NotifyLobbyLeaveUserInfo(pUser);

	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(PACKET::PktLobbyLeaveRes) }, (char*)&resPkt);

	return ERROR_CODE::NONE;
CHECK_ERR:
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{(short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(PACKET::PktLobbyLeaveRes)}, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// 로비의 룸 리스트 요청
ERROR_CODE PacketHandler::LobbyRoomList(RecvPacket & packet)
{
CHECK_START
	// 현재 로비에 있는지 조사한다.
	// 룸 리스트를 보내준다.

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// 로비에 있지 않은 유저가 로비의 룸 리스트 요청하면 에러
	if (pUser->InLobby() == false) {
		CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_DOMAIN);
	}

	// 요청한 로비가 유효하지 않은 로비면 에러
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

// 로비에 접속한 유저 리스트 요청
ERROR_CODE PacketHandler::LobbyUserList(RecvPacket & packet)
{
CHECK_START
	// 현재 로비에 있는지 조사한다.
	// 유저 리스트를 보내준다.

	auto pUserRet = mUserManager->GetUser(packet.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	// 로비에 접속하지 않은 유저가 유저 리스트 요청하면 에러
	if (pUser->InLobby() == false) {
		CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
	}

	// 로비가 유효하지 않으면 에러
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

// 룸에 접속 요청
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

	// 로비에 접속하지 않은 유저가 룸에 접속하려고 하면 에러
	if (pUser->InLobby() == false) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	// 유효하지 않은 로비면 에러
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
	}

	// 유효하지 않은 룸이면 에러
	auto pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
	if (pRoom == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
	}

	// 룸을 만드는 경우라면 룸을 만든다
	if (reqPkt->IsCreate)
	{
		auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
		if (ret != ERROR_CODE::NONE) {
			CHECK_ERROR(ret);
		}
	}

	// 룸에 유저를 입장시킴
	auto enterRet = pRoom->EnterUser(pUser);
	if (enterRet != ERROR_CODE::NONE) {
		CHECK_ERROR(enterRet);
	}

	// 유저 정보를 룸에 들어왔다고 변경한다.
	pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

	// 로비에 유저가 나갔음을 알린다
	pLobby->NotifyLobbyLeaveUserInfo(pUser);

	// 로비에 룸 정보를 통보한다.
	pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

	// 룸에 새 유저 들어왔다고 알린다
	pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());

	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt) }, (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// 룸 나가기 요청
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

	// 룸에 접속하지 않은 유저가 룸 나가기 요청하면 에러
	if (pUser->InRoom() == false) {
		CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	// 잘못된 로비면 에러
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
	}

	// 유효하지 않은 룸이면 에러
	auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
	if (pRoom == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
	}

	auto leaveRet = pRoom->LeaveUser(userIndex);
	if (leaveRet != ERROR_CODE::NONE) {
		CHECK_ERROR(leaveRet);
	}

	// 유저 정보를 로비로 변경
	pUser->EnterLobby(lobbyIndex);

	// 룸에 유저가 나갔음을 통보
	pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

	// 로비에 새로운 유저가 들어왔음을 통보
	pLobby->NotifyLobbyEnterUserInfo(pUser);

	// 로비에 바뀐 방 정보를 통보
	pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt) }, (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}

// 룸 채팅 요청
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

	// 룸에 접속하지 않은 유저가 채팅치려고 하면 에러
	if (pUser->InRoom() == false) {
		CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	// 유효하지 않은 로비면 에러
	if (pLobby == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
	}

	auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
	// 유효하지 않은 룸이면 에러
	if (pRoom == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
	}

	// 룸에 접속한 다른 유저들에게 채팅메세지 브로드캐스트
	pRoom->NotifyChat(pUser->GetSessioID(), pUser->GetID().c_str(), reqPkt->Msg);

	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt) }, (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	mNetwork->AddSendQueue(mNetwork->GetSession(packet.SessionIndex), PACKET::Header{ (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt) }, (char*)&resPkt);
	return (ERROR_CODE)__result;
}
