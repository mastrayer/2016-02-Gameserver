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


	// 로그인
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

	//- 채널 리스트 요청
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


	//- 로비 입장 요청
	struct PktLobbyEnterReq
	{
		short LobbyId;
	};

	struct PktLobbyEnterRes : Base
	{
		short MaxUserCount;
		short MaxRoomCount;
	};


	//- 로비에 있는 유저에게 로비에 들어온 유저 통보
	struct PktLobbyNewUserInfoNtf
	{
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};


	//- 로비의 룸 리스트 요청
	struct PktLobbyRoomListReq
	{
		short StartRoomIndex; // 로비에서 처음 요청할 때는 0, 두번째부터는 앞에 받은 데이터의 마지막 번호 + 1
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
		bool IsEnd = false; // true 이면 더 이상 룸 리스트 요청을 하지 않는다
		short Count = 0;
		RoomSmallInfo RoomInfo[MAX_NTF_LOBBY_ROOM_LIST_COUNT];
	};


	//- 로비의 유저 리스트 요청
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
		bool IsEnd = false; // true 이면 더 이상 룸 리스트 요청을 하지 않는다
		short Count = 0;
		UserSmallInfo UserInfo[MAX_SEND_LOBBY_USER_LIST_COUNT];
	};


	//- 로비에서 나가기 요청
	struct PktLobbyLeaveReq {};

	struct PktLobbyLeaveRes : Base
	{
	};

	//- 로비에서 나가는 유저 통보(로비에 있는 유저에게)
	struct PktLobbyLeaveUserInfoNtf
	{
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};


	//- 룸에 들어가기 요청
	struct PktRoomEnterReq
	{
		bool IsCreate;
		short RoomIndex;
		wchar_t RoomTitle[MAX_ROOM_TITLE_SIZE + 1];
	};

	struct PktRoomEnterRes : Base
	{
	};


	//- 변경된 룸 정보 통보
	struct PktChangedRoomInfoNtf
	{
		RoomSmallInfo RoomInfo;
	};

	//- 룸에 있는 유저에게 새로 들어온 유저 정보 통보
	struct PktRoomEnterUserInfoNtf
	{
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};


	//- 룸 나가기 요청
	struct PktRoomLeaveReq {};

	struct PktRoomLeaveRes : Base
	{
	};

	//- 룸에서 나가는 유저 통보(로비에 있는 유저에게)
	struct PktRoomLeaveUserInfoNtf
	{
		char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};


	//- 룸 채팅
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