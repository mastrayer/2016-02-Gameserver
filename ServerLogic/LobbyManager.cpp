#include "Packet.h"
#include "PacketError.h"

#include "Lobby.h"
#include "LobbyManager.h"

LobbyManager::LobbyManager() {}

LobbyManager::~LobbyManager() {}


// �κ� �Ŵ��� �ʱ�ȭ
void LobbyManager::Init(const LobbyManagerConfig config, Network* pNetwork)
{
	m_pRefNetwork = pNetwork;

	// �κ� Ǯ �����
	for (int i = 0; i < config.MaxLobbyCount; ++i)
	{
		Lobby lobby;
		lobby.Init((short)i, (short)config.MaxLobbyUserCount, (short)config.MaxRoomCountByLobby, (short)config.MaxRoomUserCount);
		lobby.SetNetwork(m_pRefNetwork);

		m_LobbyList.push_back(lobby);
	}
}

// �κ� ���� �޾ƿ���
Lobby* LobbyManager::GetLobby(short lobbyId)
{
	// �߸��� �κ� ID�� nullptr ����
	if (lobbyId < 0 || lobbyId >= (short)m_LobbyList.size()) {
		return nullptr;
	}

	return &m_LobbyList[lobbyId];
}

// �κ� ����Ʈ�� ����
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

	// ���� �����͸� ���̱� ���� ������� ���� LobbyListInfo ũ��� ���� ������ �ȴ�.
	m_pRefNetwork->AddSendQueue(m_pRefNetwork->GetSession(sessionIndex), PACKET::Header{ (short)PACKET_ID::LOBBY_LIST_RES, sizeof(resPkt) }, (char*)&resPkt);
}