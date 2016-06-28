#ifndef _REDIS_AGENT_H_
#define _REDIS_AGENT_H_

#include "hiredis.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#define REDIS_CONNECT_TIMEOUT 3000

class CRedisAgent
{
public:
	CRedisAgent():m_pRedisContext(NULL){}
	~CRedisAgent();
	int Initialize(const string &strAddr);
	redisReply *SendRedisCommand(const string &strCommand);
	redisReply *BinaryOperationByOneParam(const string &strFormat, const string &strParam);
	redisReply *BinaryOperationByTwoParam(const string &strFormat, const string &strParam1, const string &strParam2);
	redisReply *SaveBinaryData(const string &strKey, const string &strValue, unsigned int dwExpiration = 0);
	int AppendPipelineCommand(string *arrayStrCommand, const int len);
	int AppendCmdByOneParam(const string &strFormat, const string &strParam1);
	int AppendCmdByTwoParam(const string &strFormat, const string &strParam1, const string &strParam2);
	int AppendCmdArgv(vector<string> &vecCmdArgs);
	int GetReplyInPipeline(redisReply **reply);
	int BeginTransaction();
	redisReply *EndTransaction();
	int Ping();
	int GetError();
	char * GetErrorStr();
	redisContext *GetRedisContext(){return m_pRedisContext;}
	string GetAddr(){return m_strRedisServerAddr;}
private:
	string m_strRedisServerAddr;
	redisContext *m_pRedisContext;
};
#endif
