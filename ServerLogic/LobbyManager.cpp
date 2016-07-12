#include "Packet.h"
#include "PacketError.h"

#include "Lobby.h"
#include "LobbyManager.h"

LobbyManager::LobbyManager() {}

LobbyManager::~LobbyManager() {}


// 로비 매니저 초기화
void LobbyManager::Init(const LobbyManagerConfig config, Network* pNetwork)
{
	m_pRefNetwork = pNetwork;

	// 로비 풀 만들기
	for (int i = 0; i < config.MaxLobbyCount; ++i)
	{
		Lobby lobby;
		lobby.Init((short)i, (short)config.MaxLobbyUserCount, (short)config.MaxRoomCountByLobby, (short)config.MaxRoomUserCount);
		lobby.SetNetwork(m_pRefNetwork);

		m_LobbyList.push_back(lobby);
	}
}

// 로비 정보 받아오기
Lobby* LobbyManager::GetLobby(short lobbyId)
{
	// 잘못된 로비 ID면 nullptr 리턴
	if (lobbyId < 0 || lobbyId >= (short)m_LobbyList.size()) {
		return nullptr;
	}

	return &m_LobbyList[lobbyId];
}

// 로비 리스트들 리턴
void LobbyManager::SendLobbyListInfo(const int sessionIndex)
{
	PACKET::PktLobbyListRes resPkt;
	resPkt.ErrorCode = (short)ERROR_CODE::NONE;
	resPkt.LobbyCount = static_cast<short>(m_LobbyList.size());

	int index = 0;
	for (auto& lobby : m_LobbyList)
	{
		resPkt.LobbyList[index].LobbyId = lobby.GetIndex();
		resPkt.LobbyList[index].LobbyUserCount = lobby.GetUserCount();

		++index;
	}

	// 보낼 데이터를 줄이기 위해 사용하지 않은 LobbyListInfo 크기는 빼고 보내도 된다.
	m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(sessionIndex), PACKET::Header{ (short)PACKET_ID::LOBBY_LIST_RES, sizeof(resPkt) }, (char*)&resPkt);
}