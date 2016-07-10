#pragma once
#include <string>

class User
{
public:
	enum class STATE {
		NONE	= 0,
		LOGIN	= 1,
		LOBBY	= 2,
		ROOM	= 3,
	};

	User(const short index);
	~User();

	void Reset();
	void Set(const int sessionID, std::string ID);

	short GetIndex() { return mIndex; }
	int GetSessioID() { return mSessionID; }
	std::string& GetID() { return mID; }
	bool IsConfirm() { return mIsConfirm; }

	short GetLobbyIndex() { return mLobbyIndex; }
	short GetRoomIndex() { return mRoomIndex; }

	void EnterLobby(const short lobbyIndex);
	void LeaveLobby();

	void EnterRoom(const short lobbyIndex, const short roomIndex);

	bool InLogIn() { return mState == STATE::LOGIN; }
	bool InLobby() { return mState == STATE::LOBBY; }
	bool InRoom() { return mState == STATE::ROOM; }

protected:
	short mIndex = -1;
	int mSessionID = -1;
	std::string mID;
	bool mIsConfirm = false;
	STATE mState = STATE::NONE;
	short mLobbyIndex = -1;
	short mRoomIndex = -1;
};


