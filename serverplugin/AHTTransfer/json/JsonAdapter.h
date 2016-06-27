#ifndef _AHT_COMMON_JSON_ADAPTER_
#define _AHT_COMMON_JSON_ADAPTER_

#include "jsonMsg.h"

class CJsonAdapter
{
public:
	static int GetJsonData( CJsonDecoder &decoder, const string &strKey, string &strValue );
};

#endif
