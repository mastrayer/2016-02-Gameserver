#include <algorithm>

// 왜 헤더파일에서 인클루드하지 않고 cpp에서 하는지 잘 이해가 안됨.
// 왜 헤더파일에서 인클루드하지 않고 cpp에서 하는지 잘 이해가 안됨.
// 왜 헤더파일에서 인클루드하지 않고 cpp에서 하는지 잘 이해가 안됨.
// 왜 헤더파일에서 인클루드하지 않고 cpp에서 하는지 잘 이해가 안됨.
#include "Packet.h"
#include "PacketError.h"
#include "User.h"
#include "Lobby.h"

Lobby::Lobby() {}

Lobby::~Lobby() {}

// 로비를 초기화
void Lobby::Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount)
{
	m_LobbyIndex = lobbyIndex;
	m_MaxUserCount = (short)maxLobbyUserCount;

	// 유저 풀 만들어놓기
	for (int i = 0; i < maxLobbyUserCount; ++i)
	{
		LobbyUser lobbyUser;
		lobbyUser.Index = (short)i;
		lobbyUser.pUser = nullptr;

		m_UserList.push_back(lobbyUser);
	}

	// 룸 풀 만들어놓기
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

// 유저가 로비에 입장
ERROR_CODE Lobby::EnterUser(User* pUser)
{
	// 로비 최대 수용인원을 넘으면 에러
	if (m_UserIndexDic.size() >= m_MaxUserCount) {
		return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;
	}

	// 이미 이 로비에 있는 유저가 또 로비에 들어오려고 하면 에러
	if (FindUser(pUser->GetIndex()) != nullptr) {
		return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;
	}

	// 로비에 접속한 유저 리스트에 유저 추가
	auto addRet = AddUser(pUser);
	if (addRet != ERROR_CODE::NONE) {
		return addRet;
	}

	pUser->EnterLobby(m_LobbyIndex);

	m_UserIndexDic.insert({ pUser->GetIndex(), pUser });
	m_UserIDDic.insert({ pUser->GetID().c_str(), pUser });

	return ERROR_CODE::NONE;
}

// 유저가 로비 떠남
ERROR_CODE Lobby::LeaveUser(const int userIndex)
{
	RemoveUser(userIndex);

	auto pUser = FindUser(userIndex);

	// 로비에 유저가 없는데 나가려고 하면 에러
	if (pUser == nullptr) {
		return ERROR_CODE::LOBBY_LEAVE_USER_NVALID_UNIQUEINDEX;
	}

	pUser->LeaveLobby();

	m_UserIndexDic.erase(pUser->GetIndex());
	m_UserIDDic.erase(pUser->GetID().c_str());

	return ERROR_CODE::NONE;
}

// 로비에 접속한 유저들중 특정 유저 찾기
User* Lobby::FindUser(const int userIndex)
{
	auto findIter = m_UserIndexDic.find(userIndex);

	if (findIter == m_UserIndexDic.end()) {
		return nullptr;
	}

	return (User*)findIter->second;
}

// 유저 리스트에 유저 추가
ERROR_CODE Lobby::AddUser(User* pUser)
{
	// 유저 풀에서 사용 가능한 유저 찾기
	auto findIter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [](auto& lobbyUser) { return lobbyUser.pUser == nullptr; });

	// 유저 풀에 사용 가능한 유저가 없다
	if (findIter == std::end(m_UserList)) {
		return ERROR_CODE::LOBBY_ENTER_EMPTY_USER_LIST;
	}

	// 유저 풀 할당
	findIter->pUser = pUser;
	return ERROR_CODE::NONE;
}

// 유저 리스트에서 유저 삭제
void Lobby::RemoveUser(const int userIndex)
{
	// 유저 풀에서 userIndex 값을 가진 유저를 찾는다
	auto findIter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [userIndex](auto& lobbyUser) { return lobbyUser.pUser != nullptr && lobbyUser.pUser->GetIndex() == userIndex; });

	// ==이 되어야 하는거 아닌가...?
	if (findIter != std::end(m_UserList)) {
		return;
	}

	findIter->pUser = nullptr;
}

// 접속한 유저수 반환
short Lobby::GetUserCount()
{
	return static_cast<short>(m_UserIndexDic.size());
}


// 로비에 접속한 다른 유저들에게 새 유저 접속 브로드캐스트
void Lobby::NotifyLobbyEnterUserInfo(User* pUser)
{
	PACKET::PktLobbyNewUserInfoNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pUser->GetID().c_str(), PACKET::MAX_USER_ID_SIZE);

	SendToAllUser((short)PACKET_ID::LOBBY_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, pUser->GetIndex());
}

// 유저가 로비에서 나간걸 로비에 접속한 다른 유저들에게 브로드캐스트
void Lobby::NotifyLobbyLeaveUserInfo(User* pUser)
{
	PACKET::PktLobbyLeaveUserInfoNtf pkt;
	strncpy_s(pkt.UserID, _countof(pkt.UserID), pUser->GetID().c_str(), PACKET::MAX_USER_ID_SIZE);

	SendToAllUser((short)PACKET_ID::LOBBY_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt, pUser->GetIndex());
}

// 로비에 생성되어 있는 룸 리스트를 반환
ERROR_CODE Lobby::SendRoomList(const int sessionId, const short startRoomId)
{
	// 잘못된 룸 인덱스가 넘어오면 에러
	if (startRoomId < 0 || startRoomId >= (m_RoomList.size() - 1)) {
		return ERROR_CODE::LOBBY_ROOM_LIST_INVALID_START_ROOM_INDEX;
	}

	PACKET::PktLobbyRoomListRes pktRes;
	short roomCount = 0;
	int lastCheckedIndex = 0;

	for (int i = startRoomId; i < m_RoomList.size(); ++i)
	{
		auto& room = m_RoomList[i];

		// 활성화된 룸중에서 제일 마지막에 리스팅된 룸의 인덱스
		lastCheckedIndex = i;

		// 활성화 되어 있지 않은 룸이면 패스
		if (room.IsUsed() == false) {
			continue;
		}

		pktRes.RoomInfo[roomCount].RoomIndex = room.GetIndex();
		pktRes.RoomInfo[roomCount].RoomUserCount = room.GetUserCount();
		wcsncpy_s(pktRes.RoomInfo[roomCount].RoomTitle, PACKET::MAX_ROOM_TITLE_SIZE + 1, room.GetTitle(), PACKET::MAX_ROOM_TITLE_SIZE);

		++roomCount;

		// 한번에 보낼 수 있는 룸 리스트 갯수를 넘으면 루프 종료
		if (roomCount >= PACKET::MAX_NTF_LOBBY_ROOM_LIST_COUNT) {
			break;
		}
	}

	pktRes.Count = roomCount;

	// 이번에 보내는 룸 리스트가 마지막 페이지이면 IsEnd 값을 같이 보낸다
	if (roomCount <= 0 || (lastCheckedIndex + 1) == m_RoomList.size()) {
		pktRes.IsEnd = true;
	}

	m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(sessionId), PACKET::Header{ (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(pktRes) }, (char*)&pktRes);

	return ERROR_CODE::NONE;
}

// 로비에 접속한 유저의 리스트를 보냄
ERROR_CODE Lobby::SendUserList(const int sessionId, const short startUserIndex)
{
	// 잘못된 start Index면 에러
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

		// 로비에 없거나(룸에 입장한 경우) 비어있는 풀 인덱스면 패스
		if (lobbyUser.pUser == nullptr || lobbyUser.pUser->InLobby() == false) {
			continue;
		}

		pktRes.UserInfo[userCount].LobbyUserIndex = (short)i;
		strncpy_s(pktRes.UserInfo[userCount].UserID, PACKET::MAX_USER_ID_SIZE + 1, lobbyUser.pUser->GetID().c_str(), PACKET::MAX_USER_ID_SIZE);

		++userCount;

		// 한번에 보낼 수 있는 리스트의 유저 갯수를 넘으면 그만 보냄
		if (userCount >= PACKET::MAX_SEND_LOBBY_USER_LIST_COUNT) {
			break;
		}
	}

	pktRes.Count = userCount;

	// 유저 리스트의 마지막 페이지면 IsEnd값 설정해서 보낸다
	if (userCount <= 0 || (lastCheckedIndex + 1) == m_UserList.size()) {
		pktRes.IsEnd = true;
	}

	m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(sessionId), PACKET::Header{ (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(pktRes) }, (char*)&pktRes);

	return ERROR_CODE::NONE;
}

// 로비에 접속해있는 다른 유저들에게 브로드캐스트
void Lobby::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
{
	for (auto& pUser : m_UserIndexDic)
	{
		// passUserIndex(본인)이면 패스
		if (pUser.second->GetIndex() == passUserindex) {
			continue;
		}

		// 로비가 아니라 룸인 유저들에게도 패스
		if (pUser.second->InLobby() == false) {
			continue;
		}

		m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(pUser.second->GetSessioID()), PACKET::Header{ packetId, dataSize }, pData);
	}
}

// 룸 정보 받아오기
Room* Lobby::GetRoom(const short roomIndex)
{
	if (roomIndex < 0 || roomIndex >= m_RoomList.size()) {
		return nullptr;
	}

	return &m_RoomList[roomIndex];
}

// 룸 정보 변경을 브로드캐스트
void Lobby::NotifyChangedRoomInfo(const short roomIndex)
{
	PACKET::PktChangedRoomInfoNtf pktNtf;

	auto& room = m_RoomList[roomIndex];

	pktNtf.RoomInfo.RoomIndex = room.GetIndex();
	pktNtf.RoomInfo.RoomUserCount = room.GetUserCount();

	// 룸 설정 변경(제목변경)인가?
	if (m_RoomList[roomIndex].IsUsed()) {
		wcsncpy_s(pktNtf.RoomInfo.RoomTitle, PACKET::MAX_ROOM_TITLE_SIZE + 1, room.GetTitle(), PACKET::MAX_ROOM_TITLE_SIZE);
	}
	else { // 룸이 삭제되었나?
		pktNtf.RoomInfo.RoomTitle[0] = L'\0';
	}

	SendToAllUser((short)PACKET_ID::ROOM_CHANGED_INFO_NTF, sizeof(pktNtf), (char*)&pktNtf);
}