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
#pragma pack(pop)

}