#include "RedisTypeAgent.h"
#include "ErrorMsg.h"
#include "RedisVirtualServiceLog.h"

int CRedisTypeAgent::Del(const string &strKey)
{ 
    int returnNo = RVS_SUCESS;
    string strFormat = "DEL %b";
	
	redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

	
	if (reply->type == REDIS_REPLY_INTEGER) 
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgent::%s, Server[%s] : Del Key[%s] Success, ReplyInteger[%ld]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->integer);
		returnNo = RVS_SUCESS;
	}
	else
	{
		RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s] : Redis del key error, Key[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}
	
	freeReplyObject(reply);
	return returnNo;
}

//过期时间单位为秒
int CRedisTypeAgent::Expire(const string &strKey, unsigned int dwExpiration)
{
	int returnNo = RVS_SUCESS;
	char tempStr[32] = {0};
	snprintf(tempStr, sizeof(tempStr), "%d", dwExpiration);
	string strFormat = "EXPIRE %b ";
	strFormat += tempStr;
		
	redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
		
	if (reply->type == REDIS_REPLY_INTEGER) 
	{
		RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgent::%s, Server[%s] : expire Key[%s] Success, ReplyInteger[%ld]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->integer);
		returnNo = RVS_SUCESS;
	}
	else
	{
		RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s] : expire key error, Key[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}
		
	freeReplyObject(reply);
	return returnNo;

}

int CRedisTypeAgent::ExpireInTransaction(const string &strKey, unsigned int dwExpiration)
{
	char tempStr[32] = {0};
	snprintf(tempStr, sizeof(tempStr), "%d", dwExpiration);
	string strFormat = "EXPIRE %b ";
	strFormat += tempStr;
		
	redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
		
	if (reply->type == REDIS_REPLY_ERROR) 
	{
		RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s]  Key[%s] Expiration[%d] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), dwExpiration, reply->str);
	}
		
	freeReplyObject(reply);
	return RVS_SUCESS;
}

//获取某个键剩余的过期时间，单位为秒
int CRedisTypeAgent::Ttl(const string &strKey, int &ttl)
{
	int returnNo = RVS_SUCESS;
	string strFormat = "TTL %b ";
			
	redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
		
			
	if (reply->type == REDIS_REPLY_INTEGER) 
	{
		RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgent::%s, Server[%s] success, ReplyInteger[%ld]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), reply->integer);
		ttl = reply->integer;
		returnNo = RVS_SUCESS;
	}
	else
	{
		RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s] error, Key[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}
			
	freeReplyObject(reply);
	return returnNo;

}

int CRedisTypeAgent::BatchExpire(vector<SKeyValue> &vecKeyValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgent::%s, vecSize[%d]\n", __FUNCTION__, vecKeyValue.size());
    string strFormat = "EXPIRE %b ";
	
    vector<SKeyValue>::iterator iter = vecKeyValue.begin();
	int waitInPipe = 0 ;
	for(;iter != vecKeyValue.end(); iter++)
	{
		SKeyValue &keyValue = *iter;
		string finalFormat = strFormat + keyValue.strValue;
		int result = m_pRedisAgent->AppendCmdByOneParam(finalFormat, keyValue.strKey);
	     if(-1 == result)
	     {
	     	return RVS_REDIS_CONNECT_ERROR;
	     }
		 else if(result == 0)
		 {
		 	waitInPipe++;
		 }
	}

    iter = vecKeyValue.begin();
	for(int i = 0;i < waitInPipe && iter != vecKeyValue.end(); i++, iter++)
	{
		redisReply *reply = NULL;
		if(0 != m_pRedisAgent->GetReplyInPipeline(&reply))
		{
			return RVS_REDIS_CONNECT_ERROR;
		}
		SKeyValue &keyValue = *iter;
		if(REDIS_REPLY_INTEGER == reply->type)
		{
			char buf[33] = {0};
			snprintf(buf, sizeof(buf), "%lld", reply->integer);
			keyValue.strValue = buf;
		}
	}

	return RVS_SUCESS;
}

int CRedisTypeAgent::BatchDelete(vector<SKeyValue> &vecKeyValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgent::%s\n", __FUNCTION__);
    int returnNo = RVS_SUCESS;

    vector<string> vecCmdArgs;
	vecCmdArgs.push_back("DEL");
    vector<SKeyValue>::iterator iterKeyValue = vecKeyValue.begin();
	for(; iterKeyValue != vecKeyValue.end(); iterKeyValue++)
	{
	    SKeyValue &keyValue = *iterKeyValue;
		vecCmdArgs.push_back(keyValue.strKey);
	}

	if(0 != m_pRedisAgent->AppendCmdArgv(vecCmdArgs))
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    redisReply *reply = NULL;
    if(0 != m_pRedisAgent->GetReplyInPipeline(&reply))
    {
   	    return RVS_REDIS_CONNECT_ERROR;
    }

	if (reply->type == REDIS_REPLY_INTEGER) 
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisTypeAgent::%s, Server[%s] Success, ReplyInteger[%ld]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), reply->integer);
		returnNo = RVS_SUCESS;
	}
	else
	{
		RVS_XLOG(XLOG_ERROR, "CRedisTypeAgent::%s, Server[%s] : Redis del key error, Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}

	freeReplyObject(reply);
	return returnNo;
}

