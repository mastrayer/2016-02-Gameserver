#pragma once
#include "PacketError.h"

namespace PACKET {

#pragma pack(push, 1)
	struct Header
	{
		short ID;
		short BodySize;
	};

	struct Base
	{
		short ErrorCode = (short)ERROR_CODE::NONE;
		void SetError(ERROR_CODE error) { ErrorCode = (short)error; }
	};


	// �α���
	const int MAX_USER_ID_SIZE = 16;
	const int MAX_USER_PASSWORD_SIZE = 16;
	struct LogInRequest
	{
		char ID[MAX_USER_ID_SIZE + 1] = { 0, };
		char password[MAX_USER_PASSWORD_SIZE + 1] = { 0, };
	};

	struct LogInResponse : Base
	{
	};

	//- ä�� ����Ʈ ��û
	const int MAX_LOBBY_LIST_COUNT = 20;
	struct LobbyListInfo
	{
		short LobbyId;
		short LobbyUserCount;
	};
	struct PktLobbyListRes : Base
	{
		short LobbyCount = 0;
		LobbyListInfo LobbyList[MAX_LOBBY_LIST_COUNT];
	};


	//- �κ� ���� ��û
	struct PktLobbyEnterReq
	{
		short LobbyId;
	};

	struct PktLobbyEnterRes : Base
	{
		short MaxUserCount;
		short MaxRoomCount;
	};


	//- �κ� �ִ� �������� �κ� ���� ���� �뺸
	struct PktLobbyNewUserInfoNtf
	{
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};


	//- �κ��� �� ����Ʈ ��û
	struct PktLobbyRoomListReq
	{
		short StartRoomIndex; // �κ񿡼� ó�� ��û�� ���� 0, �ι�°���ʹ� �տ� ���� �������� ������ ��ȣ + 1
	};

	const int MAX_ROOM_TITLE_SIZE = 16;
	struct RoomSmallInfo
	{
		short RoomIndex;
		short RoomUserCount;
		wchar_t RoomTitle[MAX_ROOM_TITLE_SIZE + 1] = { 0, };
	};

	const int MAX_NTF_LOBBY_ROOM_LIST_COUNT = 12;
	struct PktLobbyRoomListRes : Base
	{
		bool IsEnd = false; // true �̸� �� �̻� �� ����Ʈ ��û�� ���� �ʴ´�
		short Count = 0;
		RoomSmallInfo RoomInfo[MAX_NTF_LOBBY_ROOM_LIST_COUNT];
	};


	//- �κ��� ���� ����Ʈ ��û
	struct PktLobbyUserListReq
	{
		short StartUserIndex;
	};

	struct UserSmallInfo
	{
		short LobbyUserIndex;
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};

	const int MAX_SEND_LOBBY_USER_LIST_COUNT = 32;
	struct PktLobbyUserListRes : Base
	{
		bool IsEnd = false; // true �̸� �� �̻� �� ����Ʈ ��û�� ���� �ʴ´�
		short Count = 0;
		UserSmallInfo UserInfo[MAX_SEND_LOBBY_USER_LIST_COUNT];
	};


	//- �κ񿡼� ������ ��û
	struct PktLobbyLeaveReq {};

	struct PktLobbyLeaveRes : Base
	{
	};

	//- �κ񿡼� ������ ���� �뺸(�κ� �ִ� ��������)
	struct PktLobbyLeaveUserInfoNtf
	{
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};


	//- �뿡 ���� ��û
	struct PktRoomEnterReq
	{
		bool IsCreate;
		short RoomIndex;
		wchar_t RoomTitle[MAX_ROOM_TITLE_SIZE + 1];
	};

	struct PktRoomEnterRes : Base
	{
	};


	//- ����� �� ���� �뺸
	struct PktChangedRoomInfoNtf
	{
		RoomSmallInfo RoomInfo;
	};

	//- �뿡 �ִ� �������� ���� ���� ���� ���� �뺸
	struct PktRoomEnterUserInfoNtf
	{
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};


	//- �� ������ ��û
	struct PktRoomLeaveReq {};

	struct PktRoomLeaveRes : Base
	{
	};

	//- �뿡�� ������ ���� �뺸(�κ� �ִ� ��������)
	struct PktRoomLeaveUserInfoNtf
	{
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};


	//- �� ä��
	const int MAX_ROOM_CHAT_MSG_SIZE = 256;
	struct PktRoomChatReq
	{
		wchar_t Msg[MAX_ROOM_CHAT_MSG_SIZE + 1] = { 0, };
	};

	struct PktRoomChatRes : Base
	{
	};

	struct PktRoomChatNtf
	{
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
		wchar_t Msg[MAX_ROOM_CHAT_MSG_SIZE + 1] = { 0, };
	};
#pragma pack(pop)

}