#include "User.h"



User::User(const short index)
{
	mIndex = index;
}


User::~User()
{
}


void User::Reset()
{
	mSessionID = 0;
	mID = "";
	mIsConfirm = false;
	mState = STATE::NONE;
	mLobbyIndex = -1;
	mRoomIndex = -1;
}

void User::Set(const int sessionID, std::string ID)
{
	mIsConfirm = true;
	mState = STATE::LOGIN;

	mSessionID = sessionID;
	mID = ID;

}

void User::EnterLobby(const short lobbyIndex)
{
	mLobbyIndex = lobbyIndex;
	mState = STATE::LOBBY;
}
void User::LeaveLobby()
{
	mState = STATE::LOGIN;
}

void User::EnterRoom(const short lobbyIndex, const short roomIndex)
{
	mLobbyIndex = lobbyIndex;
	mRoomIndex = roomIndex;
	mState = STATE::ROOM;
}