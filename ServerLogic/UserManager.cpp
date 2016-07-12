#include <algorithm>
#include "UserManager.h"
#include "User.h"
#include "Config.h"

UserManager::UserManager()
{
}

UserManager::~UserManager()
{
}

void UserManager::Init()
{
	for (short i = 0; i<SERVER_CONFIG::max_connection; ++i) {
		User user(i);

		mUserPool.push_back(user);
		mUserPoolIndex.push_back(i);
	}
}

ERROR_CODE UserManager::AddUser(const int sessionID, std::string ID)
{
	if (FindUser(ID) != nullptr)
		return ERROR_CODE::USER_MGR_ID_DUPLICATION;

	if (mUserPoolIndex.empty())
		return ERROR_CODE::USER_MGR_MAX_USER_COUNT;

	int index = mUserPoolIndex.front();
	mUserPoolIndex.pop_front();
	
	auto &user = mUserPool[index];
	user.Set(sessionID, ID);

	mUserSession.insert({ sessionID, &user });
	mUserIDs.insert({ ID, &user });

	return ERROR_CODE::NONE;
}

ERROR_CODE UserManager::RemoveUser(const int sessionID)
{
	auto user = FindUser(sessionID);

	if (user == nullptr)
		return ERROR_CODE::USER_MGR_REMOVE_INVALID_SESSION;

	mUserSession.erase(sessionID);
	mUserIDs.erase(user->GetID());
	
	mUserPoolIndex.push_back(user->GetIndex());
	mUserPool[user->GetIndex()].Reset();

	return ERROR_CODE::NONE;
}

std::tuple<ERROR_CODE, User*> UserManager::GetUser(const int sessionID)
{
	auto user = FindUser(sessionID);

	if (user == nullptr)
		return { ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr };

	if (user->IsConfirm() == false)
		return { ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr };

	return { ERROR_CODE::NONE, user };
}

User* UserManager::FindUser(const int sessionID)
{
	auto result = mUserSession.find(sessionID);

	if (result == mUserSession.end())
		return nullptr;

	return (User *)(result->second);
}

User* UserManager::FindUser(std::string ID)
{
	auto result = mUserIDs.find(ID);

	if (result == mUserIDs.end())
		return nullptr;

	return (User *)(result->second);
}