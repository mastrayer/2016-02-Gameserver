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

// �� �ʱ�ȭ
void Room::Clear()
{
	m_IsUsed = false;
	m_Title = L"";
	m_UserList.clear();
}

// �� ����
ERROR_CODE Room::CreateRoom(const wchar_t* pRoomTitle)
{
	if (m_IsUsed) {
		return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;
	}

	m_IsUsed = true;
	m_Title = pRoomTitle;

	return ERROR_CODE::NONE;
}

// �뿡 ���� ����
ERROR_CODE Room::EnterUser(User* pUser)
{
	// Ȱ��ȭ���� ���� ���� �� ����
	if (m_IsUsed == false) {
		return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
	}

	// �뿡 ������ ������ �� ���� ����
	if (m_UserList.size() == m_MaxUserCount) {
		return ERROR_CODE::ROOM_ENTER_MEMBER_FULL;
	}

	m_UserList.push_back(pUser);
	return ERROR_CODE::NONE;
}

// �뿡�� ������ ����
ERROR_CODE Room::LeaveUser(const short userIndex)
{
	// ��Ȱ��ȭ�� ���̸� ����
	if (m_IsUsed == false) {
		return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
	}

	// m_UserList���� userIndex�� ������ Index�� ���� ������ ã�´�
	auto iter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [userIndex](auto pUser) { return pUser->GetIndex() == userIndex; });
	
	// �� �뿡 �������ִ� ������ �ƴѵ� �������� �ϸ� ����
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
	// �뿡 ������ ��� �������� ��ε�ĳ��Ʈ
	for (auto pUser : m_UserList)
	{
		// passUserIndex (����)���Դ� �����͸� ������ ����
		if (pUser->GetIndex() == passUserindex) {
			continue;
		}

		m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(pUser->GetSessioID()), PACKET::Header({ packetId, dataSize }), pData);
	}
}

// �뿡 �� ������ ����
void Room::NotifyEnterUserInfo(const int userIndex, const char* pszUserID)
{
	PACKET::PktRoomEnterUserInfoNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, PACKET::MAX_USER_ID_SIZE);

	// �뿡 �������ִ� �ٸ� �����鿡�� ��ε�ĳ��Ʈ
	SendToAllUser((short)PACKET_ID::ROOM_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, userIndex);
}

// ������ �뿡�� ����
void Room::NotifyLeaveUserInfo(const char* pszUserID)
{
	// �ٵ� ���� ��Ȱ��ȭ������ ����
	if (m_IsUsed == false) {
		return;
	}

	PACKET::PktRoomLeaveUserInfoNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, PACKET::MAX_USER_ID_SIZE);

	// �뿡 �������ִ� �ٸ� �����鿡�� ��ε�ĳ��Ʈ
	SendToAllUser((short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt);
}

// ������ �޼����� �Է��ߴ�
void Room::NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg)
{
	PACKET::PktRoomChatNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, PACKET::MAX_USER_ID_SIZE);
	// �޼����� �����ڵ�� 
	wcsncpy_s(pkt.Msg, PACKET::MAX_ROOM_CHAT_MSG_SIZE + 1, pszMsg, PACKET::MAX_ROOM_CHAT_MSG_SIZE);

	SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex);
}