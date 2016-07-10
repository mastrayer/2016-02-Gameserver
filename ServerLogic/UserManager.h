#pragma once
#include <vector>
#include <unordered_map>
#include <deque>
#include <string>
#include "PacketError.h"
#include "Config.h"

class User;

class UserManager
{
public:
	UserManager();
	~UserManager();

	void Init();

	ERROR_CODE AddUser(const int sessionID, std::string ID);
	ERROR_CODE RemoveUser(const int sessionID);

	std::tuple<ERROR_CODE, User *> GetUser(const int sessionID);

private:
	User *FindUser(const int sessionID);
	User *FindUser(std::string ID);

private:
	std::vector<User>						mUserPool;
	std::deque<int>							mUserPoolIndex;

	std::unordered_map<int, User*>			mUserSession;
	std::unordered_map<std::string, User*>	mUserIDs;

};