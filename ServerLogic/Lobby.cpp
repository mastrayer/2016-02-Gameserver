#include <algorithm>

// �� ������Ͽ��� ��Ŭ������� �ʰ� cpp���� �ϴ��� �� ���ذ� �ȵ�.
// �� ������Ͽ��� ��Ŭ������� �ʰ� cpp���� �ϴ��� �� ���ذ� �ȵ�.
// �� ������Ͽ��� ��Ŭ������� �ʰ� cpp���� �ϴ��� �� ���ذ� �ȵ�.
// �� ������Ͽ��� ��Ŭ������� �ʰ� cpp���� �ϴ��� �� ���ذ� �ȵ�.
#include "Packet.h"
#include "PacketError.h"
#include "User.h"
#include "Lobby.h"

Lobby::Lobby() {}

Lobby::~Lobby() {}

// �κ� �ʱ�ȭ
void Lobby::Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount)
{
	m_LobbyIndex = lobbyIndex;
	m_MaxUserCount = (short)maxLobbyUserCount;

	// ���� Ǯ ��������
	for (int i = 0; i < maxLobbyUserCount; ++i)
	{
		LobbyUser lobbyUser;
		lobbyUser.Index = (short)i;
		lobbyUser.pUser = nullptr;

		m_UserList.push_back(lobbyUser);
	}

	// �� Ǯ ��������
	for (int i = 0; i < maxRoomCountByLobby; ++i)
	{
		m_RoomList.emplace_back(Room());
		m_RoomList[i].Init((short)i, maxRoomUserCount);
	}
}

void Lobby::SetNetwork(Network* pNetwork)
{
	m_pRefNetwork = pNetwork;

	for (auto& room : m_RoomList)
	{
		room.SetNetwork(pNetwork);
	}
}

// ������ �κ� ����
ERROR_CODE Lobby::EnterUser(User* pUser)
{
	// �κ� �ִ� �����ο��� ������ ����
	if (m_UserIndexDic.size() >= m_MaxUserCount) {
		return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;
	}

	// �̹� �� �κ� �ִ� ������ �� �κ� �������� �ϸ� ����
	if (FindUser(pUser->GetIndex()) != nullptr) {
		return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;
	}

	// �κ� ������ ���� ����Ʈ�� ���� �߰�
	auto addRet = AddUser(pUser);
	if (addRet != ERROR_CODE::NONE) {
		return addRet;
	}

	pUser->EnterLobby(m_LobbyIndex);

	m_UserIndexDic.insert({ pUser->GetIndex(), pUser });
	m_UserIDDic.insert({ pUser->GetID().c_str(), pUser });

	return ERROR_CODE::NONE;
}

// ������ �κ� ����
ERROR_CODE Lobby::LeaveUser(const int userIndex)
{
	RemoveUser(userIndex);

	auto pUser = FindUser(userIndex);

	// �κ� ������ ���µ� �������� �ϸ� ����
	if (pUser == nullptr) {
		return ERROR_CODE::LOBBY_LEAVE_USER_NVALID_UNIQUEINDEX;
	}

	pUser->LeaveLobby();

	m_UserIndexDic.erase(pUser->GetIndex());
	m_UserIDDic.erase(pUser->GetID().c_str());

	return ERROR_CODE::NONE;
}

// �κ� ������ �������� Ư�� ���� ã��
User* Lobby::FindUser(const int userIndex)
{
	auto findIter = m_UserIndexDic.find(userIndex);

	if (findIter == m_UserIndexDic.end()) {
		return nullptr;
	}

	return (User*)findIter->second;
}

// ���� ����Ʈ�� ���� �߰�
ERROR_CODE Lobby::AddUser(User* pUser)
{
	// ���� Ǯ���� ��� ������ ���� ã��
	auto findIter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [](auto& lobbyUser) { return lobbyUser.pUser == nullptr; });

	// ���� Ǯ�� ��� ������ ������ ����
	if (findIter == std::end(m_UserList)) {
		return ERROR_CODE::LOBBY_ENTER_EMPTY_USER_LIST;
	}

	// ���� Ǯ �Ҵ�
	findIter->pUser = pUser;
	return ERROR_CODE::NONE;
}

// ���� ����Ʈ���� ���� ����
void Lobby::RemoveUser(const int userIndex)
{
	// ���� Ǯ���� userIndex ���� ���� ������ ã�´�
	auto findIter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [userIndex](auto& lobbyUser) { return lobbyUser.pUser != nullptr && lobbyUser.pUser->GetIndex() == userIndex; });

	// ==�� �Ǿ�� �ϴ°� �ƴѰ�...?
	if (findIter != std::end(m_UserList)) {
		return;
	}

	findIter->pUser = nullptr;
}

// ������ ������ ��ȯ
short Lobby::GetUserCount()
{
	return static_cast<short>(m_UserIndexDic.size());
}


// �κ� ������ �ٸ� �����鿡�� �� ���� ���� ��ε�ĳ��Ʈ
void Lobby::NotifyLobbyEnterUserInfo(User* pUser)
{
	PACKET::PktLobbyNewUserInfoNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pUser->GetID().c_str(), PACKET::MAX_USER_ID_SIZE);

	SendToAllUser((short)PACKET_ID::LOBBY_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, pUser->GetIndex());
}

// ������ �κ񿡼� ������ �κ� ������ �ٸ� �����鿡�� ��ε�ĳ��Ʈ
void Lobby::NotifyLobbyLeaveUserInfo(User* pUser)
{
	PACKET::PktLobbyLeaveUserInfoNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pUser->GetID().c_str(), PACKET::MAX_USER_ID_SIZE);

	SendToAllUser((short)PACKET_ID::LOBBY_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt, pUser->GetIndex());
}

// �κ� �����Ǿ� �ִ� �� ����Ʈ�� ��ȯ
ERROR_CODE Lobby::SendRoomList(const int sessionId, const short startRoomId)
{
	// �߸��� �� �ε����� �Ѿ���� ����
	if (startRoomId < 0 || startRoomId >= (m_RoomList.size() - 1)) {
		return ERROR_CODE::LOBBY_ROOM_LIST_INVALID_START_ROOM_INDEX;
	}

	PACKET::PktLobbyRoomListRes pktRes;
	short roomCount = 0;
	int lastCheckedIndex = 0;

	for (int i = startRoomId; i < m_RoomList.size(); ++i)
	{
		auto& room = m_RoomList[i];

		// Ȱ��ȭ�� ���߿��� ���� �������� �����õ� ���� �ε���
		lastCheckedIndex = i;

		// Ȱ��ȭ �Ǿ� ���� ���� ���̸� �н�
		if (room.IsUsed() == false) {
			continue;
		}

		pktRes.RoomInfo[roomCount].RoomIndex = room.GetIndex();
		pktRes.RoomInfo[roomCount].RoomUserCount = room.GetUserCount();
		wcsncpy_s(pktRes.RoomInfo[roomCount].RoomTitle, PACKET::MAX_ROOM_TITLE_SIZE + 1, room.GetTitle(), PACKET::MAX_ROOM_TITLE_SIZE);

		++roomCount;

		// �ѹ��� ���� �� �ִ� �� ����Ʈ ������ ������ ���� ����
		if (roomCount >= PACKET::MAX_NTF_LOBBY_ROOM_LIST_COUNT) {
			break;
		}
	}

	pktRes.Count = roomCount;

	// �̹��� ������ �� ����Ʈ�� ������ �������̸� IsEnd ���� ���� ������
	if (roomCount <= 0 || (lastCheckedIndex + 1) == m_RoomList.size()) {
		pktRes.IsEnd = true;
	}

	m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(sessionId), PACKET::Header{ (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(pktRes) }, (char*)&pktRes);

	return ERROR_CODE::NONE;
}

// �κ� ������ ������ ����Ʈ�� ����
ERROR_CODE Lobby::SendUserList(const int sessionId, const short startUserIndex)
{
	// �߸��� start Index�� ����
	if (startUserIndex < 0 || startUserIndex >= (m_UserList.size() - 1)) {
		return ERROR_CODE::LOBBY_USER_LIST_INVALID_START_USER_INDEX;
	}

	int lastCheckedIndex = 0;
	PACKET::PktLobbyUserListRes pktRes;
	short userCount = 0;

	for (int i = startUserIndex; i < m_UserList.size(); ++i)
	{
		auto& lobbyUser = m_UserList[i];
		lastCheckedIndex = i;

		// �κ� ���ų�(�뿡 ������ ���) ����ִ� Ǯ �ε����� �н�
		if (lobbyUser.pUser == nullptr || lobbyUser.pUser->InLobby() == false) {
			continue;
		}

		pktRes.UserInfo[userCount].LobbyUserIndex = (short)i;
		strncpy_s(pktRes.UserInfo[userCount].UserID, PACKET::MAX_USER_ID_SIZE + 1, lobbyUser.pUser->GetID().c_str(), PACKET::MAX_USER_ID_SIZE);

		++userCount;

		// �ѹ��� ���� �� �ִ� ����Ʈ�� ���� ������ ������ �׸� ����
		if (userCount >= PACKET::MAX_SEND_LOBBY_USER_LIST_COUNT) {
			break;
		}
	}

	pktRes.Count = userCount;

	// ���� ����Ʈ�� ������ �������� IsEnd�� �����ؼ� ������
	if (userCount <= 0 || (lastCheckedIndex + 1) == m_UserList.size()) {
		pktRes.IsEnd = true;
	}

	m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(sessionId), PACKET::Header{ (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(pktRes) }, (char*)&pktRes);

	return ERROR_CODE::NONE;
}

// �κ� �������ִ� �ٸ� �����鿡�� ��ε�ĳ��Ʈ
void Lobby::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
{
	for (auto& pUser : m_UserIndexDic)
	{
		// passUserIndex(����)�̸� �н�
		if (pUser.second->GetIndex() == passUserindex) {
			continue;
		}

		// �κ� �ƴ϶� ���� �����鿡�Ե� �н�
		if (pUser.second->InLobby() == false) {
			continue;
		}

		m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(pUser.second->GetSessioID()), PACKET::Header{ packetId, dataSize }, pData);
	}
}

// �� ���� �޾ƿ���
Room* Lobby::GetRoom(const short roomIndex)
{
	if (roomIndex < 0 || roomIndex >= m_RoomList.size()) {
		return nullptr;
	}

	return &m_RoomList[roomIndex];
}

// �� ���� ������ ��ε�ĳ��Ʈ
void Lobby::NotifyChangedRoomInfo(const short roomIndex)
{
	PACKET::PktChangedRoomInfoNtf pktNtf;

	auto& room = m_RoomList[roomIndex];

	pktNtf.RoomInfo.RoomIndex = room.GetIndex();
	pktNtf.RoomInfo.RoomUserCount = room.GetUserCount();

	// �� ���� ����(���񺯰�)�ΰ�?
	if (m_RoomList[roomIndex].IsUsed()) {
		wcsncpy_s(pktNtf.RoomInfo.RoomTitle, PACKET::MAX_ROOM_TITLE_SIZE + 1, room.GetTitle(), PACKET::MAX_ROOM_TITLE_SIZE);
	}
	else { // ���� �����Ǿ���?
		pktNtf.RoomInfo.RoomTitle[0] = L'\0';
	}

	SendToAllUser((short)PACKET_ID::ROOM_CHANGED_INFO_NTF, sizeof(pktNtf), (char*)&pktNtf);
}