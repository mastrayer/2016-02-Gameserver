#include <algorithm>

#include "Packet.h"
#include "PacketError.h"

#include "User.h"
#include "Room.h"


Room::Room() {}

Room::~Room() {}


void Room::Init(const short index, const short maxUserCount)
{
	m_Index = index;
	m_MaxUserCount = maxUserCount;
}

void Room::SetNetwork(Network* pNetwork)
{
	m_pRefNetwork = pNetwork;
}

// 룸 초기화
void Room::Clear()
{
	m_IsUsed = false;
	m_Title = L"";
	m_UserList.clear();
}

// 룸 생성
ERROR_CODE Room::CreateRoom(const wchar_t* pRoomTitle)
{
	if (m_IsUsed) {
		return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;
	}

	m_IsUsed = true;
	m_Title = pRoomTitle;

	return ERROR_CODE::NONE;
}

// 룸에 유저 입장
ERROR_CODE Room::EnterUser(User* pUser)
{
	// 활성화되지 않은 룸일 때 에러
	if (m_IsUsed == false) {
		return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
	}

	// 룸에 접속한 유저가 꽉 차면 에러
	if (m_UserList.size() == m_MaxUserCount) {
		return ERROR_CODE::ROOM_ENTER_MEMBER_FULL;
	}

	m_UserList.push_back(pUser);
	return ERROR_CODE::NONE;
}

// 룸에서 유저가 나감
ERROR_CODE Room::LeaveUser(const short userIndex)
{
	// 비활성화된 룸이면 에러
	if (m_IsUsed == false) {
		return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
	}

	// m_UserList에서 userIndex와 동일한 Index를 가진 유저를 찾는다
	auto iter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [userIndex](auto pUser) { return pUser->GetIndex() == userIndex; });
	
	// 이 룸에 접속해있던 유저가 아닌데 나가려고 하면 에러
	if (iter == std::end(m_UserList)) {
		return ERROR_CODE::ROOM_LEAVE_NOT_MEMBER;
	}

	m_UserList.erase(iter);

	if (m_UserList.empty())
	{
		Clear();
	}

	return ERROR_CODE::NONE;
}

void Room::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
{
	// 룸에 접속한 모든 유저에게 브로드캐스트
	for (auto pUser : m_UserList)
	{
		// passUserIndex (본인)에게는 데이터를 보내지 않음
		if (pUser->GetIndex() == passUserindex) {
			continue;
		}

		m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(pUser->GetSessioID()), PACKET::Header({ packetId, dataSize }), pData);
	}
}

// 룸에 새 유저가 입장
void Room::NotifyEnterUserInfo(const int userIndex, const char* pszUserID)
{
	PACKET::PktRoomEnterUserInfoNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, PACKET::MAX_USER_ID_SIZE);

	// 룸에 접속해있던 다른 유저들에게 브로드캐스트
	SendToAllUser((short)PACKET_ID::ROOM_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, userIndex);
}

// 유저가 룸에서 나감
void Room::NotifyLeaveUserInfo(const char* pszUserID)
{
	// 근데 룸이 비활성화였으면 무시
	if (m_IsUsed == false) {
		return;
	}

	PACKET::PktRoomLeaveUserInfoNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, PACKET::MAX_USER_ID_SIZE);

	// 룸에 접속해있던 다른 유저들에게 브로드캐스트
	SendToAllUser((short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt);
}

// 유저가 메세지를 입력했다
void Room::NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg)
{
	PACKET::PktRoomChatNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, PACKET::MAX_USER_ID_SIZE);
	// 메세지는 유니코드로 
	wcsncpy_s(pkt.Msg, PACKET::MAX_ROOM_CHAT_MSG_SIZE + 1, pszMsg, PACKET::MAX_ROOM_CHAT_MSG_SIZE);

	SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex);
}