#ifndef _RVS_REDIS_MSG_H
#define _RVS_REDIS_MSG_H

#include <map>
#include <string>
#include <detail/_time.h>

using std::map;
using std::string;

namespace redis{
	
typedef map<unsigned int, unsigned int> REDIS_SET_TIME_MAP;
typedef map<unsigned int, unsigned int> CODE_TYPE_MAP;
typedef map<unsigned int, CODE_TYPE_MAP> SERVICE_CODE_TYPE_MAP;

#define		BASE64_CAPACITY		1024
#define		TIMEOUT_INTERVAL	3
#define		MAX_ASYN_BUFFER		1280
#define		VALUE_SPLITTER		"|^_^|"
#define     CODE_VALUE_SPLITTER "#"

#define		GUID_CODE			9
#define		CONHASH_VIRTUAL_NODE 10
#define		MAX_NODE_NUM		 10

#define    EXPIRE_TIME_IDENTIFIER    "expire_time" 
#define    INDEX_TYPE_IDENTIFIER     "index_type"
#define    START_TYPE_IDENTIFIER     "start_type"
#define    END_TYPE_IDENTIFIER       "end_type"
#define    COUNT_TYPE_IDENTIFIER     "count_type"
#define    INCBY_TYPE_IDENTIFIER     "incby_type"
#define    ENCODED_KEY_TYPE          "encoded_key_type"
#define    ENCODED_FIELD_TYPE        "encoded_field_type"
#define    ENCODED_VALUE_TYPE        "encoded_value_type"

typedef struct stRVSKeyValue
{
	unsigned int dwKey;
	string strValue;

	int operator <(const stRVSKeyValue &p)const
	{
		return ( this->dwKey < p.dwKey);
	}
}RVSKeyValue;

typedef struct stKeyValue
{
	string strKey;
	string strValue;
}SKeyValue;

typedef struct stKeyFieldValue
{
	string strKey;
	string strField;
	string strValue;
}SKeyFieldValue;

template<class Archive>
void serialize(Archive & ar, RVSKeyValue & objKeyValue, const unsigned int version);

typedef enum enRedisType
{
        RVS_REDIS_STRING=1,
		RVS_REDIS_SET=2,
		RVS_REDIS_ZSET=3,
		RVS_REDIS_LIST=4,
		RVS_REDIS_HASH=5,
		RVS_REDIS_ADD_SERVER=6,
		RVS_REDIS_TYPE_ALL
}ERedisType;

typedef struct stRedisTypeMsg
{
    ERedisType m_enRedisType;
	unsigned int dwServiceId;
	unsigned int dwMsgId;
	unsigned int dwSequenceId;

	void *pBuffer;
	unsigned int dwLen;

	void *handler;
	string strGuid;

	struct timeval_a tStart;
	float fSpent;

	stRedisTypeMsg(ERedisType eRedisType):m_enRedisType(eRedisType),dwServiceId(0), dwMsgId(0), dwSequenceId(0), pBuffer(NULL){}
	virtual ~stRedisTypeMsg(){if(pBuffer != NULL){free(pBuffer); pBuffer = NULL;}}
}SReidsTypeMsg;

typedef struct stRedisStringMsg : public stRedisTypeMsg
{
    stRedisStringMsg():stRedisTypeMsg(RVS_REDIS_STRING){}
	~stRedisStringMsg(){}
}SRedisStringMsg;

typedef struct stRedisSetMsg : public stRedisTypeMsg
{
    stRedisSetMsg():stRedisTypeMsg(RVS_REDIS_SET){}
	~stRedisSetMsg(){}
}SRedisSetMsg;

typedef struct stRedisZsetMsg : public stRedisTypeMsg
{
    stRedisZsetMsg():stRedisTypeMsg(RVS_REDIS_ZSET){}
	~stRedisZsetMsg(){}
}SRedisZsetMsg;


typedef struct stRedisListMsg : public stRedisTypeMsg
{
    stRedisListMsg():stRedisTypeMsg(RVS_REDIS_LIST){}
	~stRedisListMsg(){}
}SRedisListMsg;

typedef struct stRedisHashMsg : public stRedisTypeMsg
{
    stRedisHashMsg():stRedisTypeMsg(RVS_REDIS_HASH){}
	~stRedisHashMsg(){}
}SRedisHashMsg;

typedef struct stRedisAddServerMsg : public stRedisTypeMsg
{
   string strAddr;
   stRedisAddServerMsg():stRedisTypeMsg(RVS_REDIS_ADD_SERVER){}
   ~stRedisAddServerMsg(){}
}SRedisAddServerMsg;

}
#endif
