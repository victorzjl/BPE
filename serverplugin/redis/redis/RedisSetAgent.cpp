#include "RedisSetAgent.h"
#include "hiredis.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"

CRedisSetAgent::~CRedisSetAgent()
{ 
}

int CRedisSetAgent::HasMember(const string &strKey, const string &strValue)
{
    int returnNo = RVS_SUCESS;
    string strFormat = "SISMEMBER %b %b";
	
	redisReply * reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);
	if(NULL == reply)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

	if(reply->type == REDIS_REPLY_INTEGER)
	{
	    if(reply->integer == 0)
	    {
	    	returnNo = RVS_REDIS_KEY_NOT_FOUND;
	    }
		else
		{
			returnNo = RVS_SUCESS;
		}
	}
	else
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] Key[%s] Value[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}

	freeReplyObject(reply);
	return returnNo;
	
}

//数据过期时间为秒
int CRedisSetAgent::Set(const string &strKey, const string &strValue, int &result, unsigned int dwExpiration)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, strKey[%s], strValue[%s], dwExpiration[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), dwExpiration);
	
	//开始事务
	int returnNo = RVS_SUCESS;
    returnNo = m_pRedisAgent->BeginTransaction();
	if(returnNo != RVS_SUCESS)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s failed ErrorNo[%d]\n", __FUNCTION__, returnNo);
		return returnNo;
	}

	returnNo = SaveBinaryDataInTransaction(strKey, strValue);
	if(returnNo == RVS_REDIS_CONNECT_ERROR)
	{
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(dwExpiration > 0)
	{
		ExpireInTransaction(strKey, dwExpiration);
	}

    //结束事务并分析执行结果
	redisReply *reply = NULL;
	reply = m_pRedisAgent->EndTransaction();
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_ERROR == reply->type)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] strKey[%s] ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}
	else if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, strKey[%s] ReplySize[%d] sucess\n", __FUNCTION__, strKey.c_str(), reply->elements);
		redisReply *childReply = reply->element[0];
		if(childReply->type == REDIS_REPLY_INTEGER)
		{
			result = childReply->integer;
		}
		returnNo = RVS_SUCESS;
	}
	
	freeReplyObject(reply);
	return returnNo;

}

int CRedisSetAgent::SaveBinaryDataInTransaction(const string &strKey,const string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, strKey[%s], strValue[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());
		
	string strFormat = "SADD %b %b";
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);
		
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_ERROR == reply->type)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] strKey[%s] strValue[%s] ReplyError[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str(), reply->str);
	}
	freeReplyObject(reply);
	return RVS_SUCESS;
}

int CRedisSetAgent::Delete(const string &strKey, const string &strValue)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
    string strFormat = "SREM %b %b";
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);

	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER != reply->type)
	{
	    returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] Key[%s] Value[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str(), reply->str);
    }
	else
	{
		RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, Server[%s] Key[%s] Value[%s] success, response[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str(), reply->integer);
	}
	
	freeReplyObject(reply);
	return returnNo;
}

int CRedisSetAgent::Size(const string &strKey, int &size)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	string strFormat = "SCARD %b";
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER != reply->type)
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] Key[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
	else
	{
	    if(reply->integer == 0)
	    {
	    	returnNo = RVS_REDIS_KEY_NOT_FOUND;
	    }
		else
		{
			size = reply->integer;
		}
		RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, Server[%s] Key[%s] success, response[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->integer);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

int CRedisSetAgent::GetAll(const string &strKey, vector<string> &vecValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	string strFormat = "SMEMBERS %b";
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
		
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
		
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, strKey[%s], ReplySize[%d]\n", __FUNCTION__, strKey.c_str(), reply->elements);
	    if(reply->elements == 0)
	    {
	        RVS_XLOG(XLOG_DEBUG, "CRedisSetAgent::%s, strKey[%s] not found\n", __FUNCTION__, strKey.c_str());
	    	returnNo = RVS_REDIS_KEY_NOT_FOUND;
	    }
		else
		{
			string temp;
		    for(unsigned int i = 0;i < reply->elements; i++)
		    {
			    redisReply* childReply = reply->element[i];
			    if(childReply->type == REDIS_REPLY_INTEGER)
			    {
			 	    char szTemp[32] = {0};
				    snprintf(szTemp, sizeof(szTemp), "%lld", childReply->integer);
				    temp = szTemp;
			    }
		        else
		        {
		            temp = childReply->str;
		        }
			    vecValue.push_back(temp);
		    }
		}  
	}	
	else
	{
	    returnNo = RVS_SYSTEM_ERROR;
	    RVS_XLOG(XLOG_ERROR, "CRedisSetAgent::%s, Server[%s] Key[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
			
	freeReplyObject(reply);
	return returnNo;
}

int CRedisSetAgent::Inter()
{
    return RVS_SUCESS;
}

int CRedisSetAgent::Union()
{
	return RVS_SUCESS;
}

