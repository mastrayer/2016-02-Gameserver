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


	// ·Î±×ÀÎ
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
#pragma pack(pop)

}