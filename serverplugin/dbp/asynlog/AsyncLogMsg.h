#ifndef _H_ASYNC_LOG_MSG_H_
#define _H_ASYNC_LOG_MSG_H_

#include "DbMsg.h"
#include "string"
using namespace std;

#define MAX_BUFFER_SIZE 0x81920

const int e_ASYN_LOG = 1;

struct SAsyncLogMsg
{
	int enType;
	SDbQueryMsg* pQueryMsg;

};



#endif


