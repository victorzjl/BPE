#include "RedisAgent.h"
#include "ErrorMsg.h"
#include "RedisVirtualServiceLog.h"

#include <sys/time.h>

int CRedisAgent::Initialize(const string &strAddr)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s, strAddr[%s]\n", __FUNCTION__, strAddr.c_str());
	
	struct timeval tv;
	tv.tv_sec = REDIS_CONNECT_TIMEOUT / 1000;
	tv.tv_usec = (REDIS_CONNECT_TIMEOUT - tv.tv_sec * 1000) * 1000;

    size_t npos = strAddr.find(":");
	if(npos == string::npos)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer[%s] address format error\n", __FUNCTION__, strAddr.c_str());
		return -1;
	}

	string strRedisServerIp = strAddr.substr(0, npos);
	int dwRedisServerPort = atoi(strAddr.substr(npos+1).c_str());

	m_strRedisServerAddr = strAddr;
	m_pRedisContext = redisConnectWithTimeout(strRedisServerIp.c_str(),dwRedisServerPort,tv);
	if (m_pRedisContext->err) 
	{
	   RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer[%s] connect error, Error[%s]\n", __FUNCTION__, strAddr.c_str(), m_pRedisContext->errstr);
	   redisFree(m_pRedisContext);
	   m_pRedisContext = NULL;
	   return -1;
	}
	
	return 0;
}

redisReply *CRedisAgent::SendRedisCommand(const string &strCommand)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s, strCommand[%s]\n", __FUNCTION__, strCommand.c_str());
	
    if(NULL == m_pRedisContext)
    {
    	RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return NULL;
    }
	
    redisReply * reply = NULL;
	reply = (redisReply *)redisCommand(m_pRedisContext, strCommand.c_str());
	return reply;
}

redisReply *CRedisAgent::BinaryOperationByOneParam(const string &strFormat, const string &strParam)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s, strFormat[%s], strParam[%s]\n", __FUNCTION__, strFormat.c_str(), strParam.c_str());
			
	if(NULL == m_pRedisContext)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return NULL;
	}
	
	redisReply * reply = NULL;
	reply = (redisReply *)redisCommand(m_pRedisContext, strFormat.c_str(), strParam.c_str(), strParam.length());
	return reply;
}

redisReply *CRedisAgent::BinaryOperationByTwoParam(const string &strFormat, const string &strParam1, const string &strParam2)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s, strFormat[%s], strParam1[%s], strParam2[%s]\n", __FUNCTION__, strFormat.c_str(), strParam1.c_str(), strParam2.c_str());
				
	if(NULL == m_pRedisContext)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return NULL;
	}
		
	redisReply * reply = NULL;
	reply = (redisReply *)redisCommand(m_pRedisContext, strFormat.c_str(), 
		strParam1.c_str(), strParam1.length(),
		strParam2.c_str(), strParam2.length());
	return reply;
}

//过期时间为秒
redisReply *CRedisAgent::SaveBinaryData(const string &strKey, const string &strValue, unsigned int dwExpiration)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s, Key[%s], Value[%d:%s]\n", __FUNCTION__, strKey.c_str(), strValue.length(), strValue.c_str());
		
	if(NULL == m_pRedisContext)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return NULL;
	}

    string strFormat;
    if(dwExpiration != 0)
   	{
    	char tempStr[32] = {0};
	    snprintf(tempStr, sizeof(tempStr), " %d", dwExpiration);
	    strFormat = "SETEX " + strKey + tempStr + " %b";
    }
	else
	{
		strFormat = "SET " + strKey + " %b";
	}
	
	redisReply * reply = NULL;
	reply = (redisReply *)redisCommand(m_pRedisContext, strFormat.c_str(), strValue.c_str(), strValue.length());
	return reply;

}

int CRedisAgent::AppendPipelineCommand(string *arrayStrCommand, const int len)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s\n", __FUNCTION__);
	if(NULL == m_pRedisContext)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return -1;
	}

	for(int i = 0; i < len; i++)
	{
		if(!arrayStrCommand[i].empty() && REDIS_OK != redisAppendCommand(m_pRedisContext, arrayStrCommand[i].c_str()))
		{
			RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, Adding Command Failed\n", __FUNCTION__, arrayStrCommand[i].c_str());
			return -1;
		}
	}
	return 0;
}

int CRedisAgent::AppendCmdByOneParam(const string &strFormat, const string &strParam1)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s, strFormat[%s] strParam1[%s]\n", __FUNCTION__, strFormat.c_str(), strParam1.c_str());
	if(NULL == m_pRedisContext)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return -1;
	}

	if(strFormat.empty() || strParam1.empty())
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, Param is wrong, strFormat[%s] strParam1[%s] strParam2[%s]\n", __FUNCTION__, strFormat.c_str(), strParam1.c_str());
		return -2;
	}
	
	//specifier in strFormat is %b 
	if(REDIS_OK != redisAppendCommand(m_pRedisContext, strFormat.c_str(), strParam1.c_str(), strParam1.length()))
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, Adding Command Failed, strFormat[%s] strParam1[%s] strParam2[%s]\n", __FUNCTION__, strFormat.c_str(), strParam1.c_str());
		return -1;
	}
	
	return 0;
}

int CRedisAgent::AppendCmdByTwoParam(const string &strFormat, const string &strParam1, const string &strParam2)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s, strFormat[%s] strParam1[%s] strParam2[%s]\n", __FUNCTION__, strFormat.c_str(), strParam1.c_str(), strParam2.c_str());
	if(NULL == m_pRedisContext)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return -1;
	}

	if(strFormat.empty() || strParam1.empty() || strParam2.empty())
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, Param is wrong, strFormat[%s] strParam1[%s] strParam2[%s]\n", __FUNCTION__, strFormat.c_str(), strParam1.c_str(), strParam2.c_str());
		return -2;
	}

	//specifier in strFormat is %b 
	if(REDIS_OK != redisAppendCommand(m_pRedisContext, strFormat.c_str(), strParam1.c_str(), strParam1.length(), strParam2.c_str(), strParam2.length()))
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, Adding Command Failed, strFormat[%s] strParam1[%s] strParam2[%s]\n", __FUNCTION__, strFormat.c_str(), strParam1.c_str(), strParam2.c_str());
		return -1;
	}
	
	return 0;
}

int CRedisAgent::AppendCmdArgv(vector<string> &vecCmdArgs)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s\n", __FUNCTION__);
	if(NULL == m_pRedisContext)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return -1;
	}

    if(vecCmdArgs.size() < 1)
    {
		return 0;
    }

	int size = vecCmdArgs.size();
    const char **argv = (const char **)malloc(sizeof(char *)*size);
	size_t *argvLen = (size_t *)malloc(sizeof(size_t)*size);
    vector<string>::iterator iter = vecCmdArgs.begin();
	for(int i=0; iter != vecCmdArgs.end(); iter++,i++)
	{
		string &str = *iter;
		int len = str.length();
		argvLen[i] = len;
		argv[i] = (char *)malloc(sizeof(char)*(len+1));
		strcpy(const_cast<char *>(argv[i]), str.c_str());
	}
	
	if(REDIS_OK != redisAppendCommandArgv(m_pRedisContext, size, argv, argvLen))
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, Adding Command Failed\n", __FUNCTION__);
		return -1;
	}
	
	for(int i = 0 ; i < size; i++)
	{
		free(const_cast<char *>(argv[i]));
	}
	free(argvLen);
	free(argv);
	return 0;
}


int CRedisAgent::GetReplyInPipeline(redisReply **reply)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s\n", __FUNCTION__);
	if(NULL == m_pRedisContext)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return -1;
	}
	
	if (REDIS_OK != redisGetReply(m_pRedisContext, (void**)reply)) 
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, Get Reply Error\n", __FUNCTION__);
		freeReplyObject(reply);
		*reply = NULL;
		return -1;
	}
	
    return 0;
}

int CRedisAgent::BeginTransaction()
{
	RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s\n", __FUNCTION__);
		
	string strCommand = "MULTI";
	redisReply *reply = SendRedisCommand(strCommand);
	
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_strRedisServerAddr.c_str(), m_pRedisContext->err, m_pRedisContext->errstr);
		return RVS_REDIS_CONNECT_ERROR;
	}

	int returnNo = RVS_SYSTEM_ERROR;
	if(reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str,"OK") == 0)
	{
		returnNo = RVS_SUCESS;
	}
	else
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s error, Error[%s]\n", __FUNCTION__, reply->str);
	}
	
	freeReplyObject(reply);
	return returnNo;
}

redisReply *CRedisAgent::EndTransaction()
{
	RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s\n", __FUNCTION__);
    string strCommand = "EXEC";
	redisReply *reply = SendRedisCommand(strCommand);
	
	return reply;
}

int CRedisAgent::Ping()
{
    RVS_XLOG(XLOG_DEBUG, "CRedisAgent::%s\n", __FUNCTION__);
	
    string strCommand = "PING";
	redisReply *reply = SendRedisCommand(strCommand);
   
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_strRedisServerAddr.c_str(), m_pRedisContext->err, m_pRedisContext->errstr);
		return RVS_REDIS_CONNECT_ERROR;
	}

    int returnNo = -1;
	if(reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str,"PONG") == 0)
	{
		returnNo = 0;
	}
	else
	{
		RVS_XLOG(XLOG_ERROR, "CRedisAgent::%s, RedisServer ping error, Error[%s]\n", __FUNCTION__, reply->str);
	}
	
	freeReplyObject(reply);
	return returnNo;
}

int CRedisAgent::GetError()
{
    if(NULL == m_pRedisContext)
    {
    	return 0;
    }
	return m_pRedisContext->err;
}

char * CRedisAgent::GetErrorStr()
{
	if(NULL == m_pRedisContext)
	{
		return NULL;
	}
	return m_pRedisContext->errstr;

}


CRedisAgent::~CRedisAgent()
{
    if(NULL != m_pRedisContext)
	{
		redisFree(m_pRedisContext);
    }
}
