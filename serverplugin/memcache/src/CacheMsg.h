#ifndef _CVS_CACHE_MSG_H_
#define _CVS_CACHE_MSG_H_

#include <string>
#include <vector>
#include <map>

#include <detail/_time.h>

using std::string;
using std::vector;
using std::map;
using std::make_pair;

typedef map<unsigned int, unsigned int> CACHE_SET_TIME_MAP;
typedef map<unsigned int, unsigned int> TYPE_CODE_MAP;
typedef map<unsigned int, TYPE_CODE_MAP> SERVICE_TYPE_CODE_MAP;


#define		BASE64_CAPACITY		1024
#define		TIMEOUT_INTERVAL	3
#define		MAX_ASYN_BUFFER		1280
#define		VALUE_SPLITTER		"^_^"

#define		GUID_CODE			9
#define		CONHASH_VIRTUAL_NODE 10
#define		MAX_NODE_NUM		 10

typedef enum enCacheOperator
{
	CVS_CACHE_GET = 1,
	CVS_CACHE_SET = 2,
	CVS_CACHE_DELETE = 3,
	CVS_CACHE_GET_DELETE = 4,
	CVS_CACHE_GET_CAS = 5,
	CVS_CACHE_ADD_SERVER = 6,
	CVS_CACHE_SET_ONLY = 7,
	CVS_ALL
}ECacheOperator;

typedef struct stCVSKeyValue
{
	unsigned int dwKey;
	string strValue;

	int operator <(const stCVSKeyValue &p)const
	{
		return ( this->dwKey < p.dwKey);
	}
}CVSKeyValue;

typedef struct stCacheMsg
{
	ECacheOperator m_enCacheOperator;
	unsigned int dwServiceId;
	unsigned int dwMsgId;
	unsigned int dwSequenceId;

	void *pBuffer;
	unsigned int dwLen;

	void *handler;
	string strGuid;

	struct timeval_a tStart;	
	
	stCacheMsg(ECacheOperator enCacheOp):m_enCacheOperator(enCacheOp),dwServiceId(0), dwMsgId(0), dwSequenceId(0), pBuffer(NULL){}
	virtual ~stCacheMsg(){if(pBuffer != NULL){free(pBuffer); pBuffer = NULL;}}
}SCacheMsg;

typedef struct stCacheGetMsg : public stCacheMsg
{
	stCacheGetMsg():stCacheMsg(CVS_CACHE_GET){}
	virtual ~stCacheGetMsg(){}
}SCacheGetMsg;

typedef struct stCacheSetMsg : public stCacheMsg
{
	stCacheSetMsg():stCacheMsg(CVS_CACHE_SET){}
	virtual ~stCacheSetMsg(){}
}SCacheSetMsg;

typedef struct stCacheDeleteMsg : public stCacheMsg
{
	stCacheDeleteMsg():stCacheMsg(CVS_CACHE_DELETE){}
	virtual ~stCacheDeleteMsg(){}
}SCacheDeleteMsg;

typedef struct stCacheGetDeleteMsg : public stCacheMsg
{
	stCacheGetDeleteMsg():stCacheMsg(CVS_CACHE_GET_DELETE){}
	virtual ~stCacheGetDeleteMsg(){}
}SCacheGetDeleteMsg;

typedef struct stCacheGetCasMsg : public stCacheMsg
{
	stCacheGetCasMsg():stCacheMsg(CVS_CACHE_GET_CAS){}
	virtual ~stCacheGetCasMsg(){}
}SCacheGetCasMsg;

typedef struct stCacheSetOnlyMsg : public stCacheMsg
{
	stCacheSetOnlyMsg() :stCacheMsg(CVS_CACHE_SET_ONLY){}
	virtual ~stCacheSetOnlyMsg(){}
}SCacheSetOnlyMsg;

typedef struct stCacheAddServerMsg : public stCacheMsg
{
	string strAddr;
	vector<unsigned int> dwVecServiceId;
	stCacheAddServerMsg():stCacheMsg(CVS_CACHE_ADD_SERVER){}
	virtual ~stCacheAddServerMsg(){}
}SCacheAddServerMsg;


#endif
