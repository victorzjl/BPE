#include "RedisStringAgent.h"
#include "hiredis.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"

using namespace redis;

CRedisStringAgent::~CRedisStringAgent()
{ 
}

int CRedisStringAgent::Get(const string &strKey, string &strValue)
{
    int returnNo = RVS_SUCESS;
    string strFormat = "GET %b";
	
	redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisStringAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

	if (reply->type == REDIS_REPLY_STRING)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisStringAgent::%s, ReplyStr[%s] ReplyLength[%d]\n", __FUNCTION__, reply->str, reply->len);
	    strValue = string(reply->str, reply->len);  
    }
	else if(reply->type == REDIS_REPLY_NIL)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisStringAgent::%s, Server[%s] : Redis get key not found, Key[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str());
		returnNo = RVS_REDIS_KEY_NOT_FOUND;
	}
	else
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisStringAgent::%s, Server[%s] : Redis get key error, Key[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}

	freeReplyObject(reply);
	return returnNo;
}

//数据过期时间为秒
int CRedisStringAgent::Set(const string &strKey, const string &strValue, unsigned int dwExpiration)
{
    int returnNo = RVS_SUCESS;
	string strFormat;
	 if(dwExpiration > 0)
   	{
    	char tempStr[32] = {0};
	    snprintf(tempStr, sizeof(tempStr), "%d", dwExpiration);
	    strFormat = "SETEX %b ";
		strFormat += tempStr;
		strFormat += " %b";
    }
	else
	{
		strFormat = "SET %b %b";
	}
	redisReply * reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisStringAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if (reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str,"OK") == 0) 
	{
		returnNo = RVS_SUCESS;
	}
	else
	{
		RVS_XLOG(XLOG_ERROR, "CRedisStringAgent::%s, Server[%s] : Redis set key error, Key[%s], Value[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}
	
	freeReplyObject(reply);
	return returnNo;

}


int CRedisStringAgent::Delete(const string &strKey)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisStringAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
    return Del(strKey);
}

int CRedisStringAgent::Incby(const string &strKey, const int incbyNum)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisStringAgent::%s, strKey[%s] Increment[%d]\n", __FUNCTION__, strKey.c_str(), incbyNum);
    int returnNo = RVS_SUCESS;
	char szTemp[16] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", incbyNum);
    string strFormat = "INCRBY %b ";
	strFormat += szTemp;
	
	redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisStringAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

	if (reply->type == REDIS_REPLY_INTEGER)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisStringAgent::%s, Server[%s] : Key[%s] Increment[%d] success\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), incbyNum); 
    }
	else
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisStringAgent::%s, Server[%s] : Key[%s] Increment[%d], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), incbyNum, reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}

	freeReplyObject(reply);
	return returnNo;
}

int CRedisStringAgent::BatchQuery(vector<SKeyValue> &vecKeyValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgent::%s\n", __FUNCTION__);
    int returnNo = RVS_SUCESS;

    vector<string> vecCmdArgs;
	vecCmdArgs.push_back("MGET");
    vector<SKeyValue>::iterator iterKeyValue = vecKeyValue.begin();
	for(; iterKeyValue != vecKeyValue.end(); iterKeyValue++)
	{
	    SKeyValue &keyValue = *iterKeyValue;
		vecCmdArgs.push_back(keyValue.strKey);
	}

	if(0 != m_pRedisAgent->AppendCmdArgv(vecCmdArgs))
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisStringAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    redisReply *reply = NULL;
    if(0 != m_pRedisAgent->GetReplyInPipeline(&reply))
    {
   	    return RVS_REDIS_CONNECT_ERROR;
    }

	if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisStringAgent::%s, Server[%s] : Size[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), reply->elements);
        if(reply->elements == 0)
        {
        	returnNo = RVS_REDIS_KEY_NOT_FOUND;
		    RVS_XLOG(XLOG_DEBUG, "CRedisStringAgent::%s, Server[%s] : Get key not found\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str());
        }
		else
		{
			iterKeyValue = vecKeyValue.begin();
		    for(unsigned int i = 0; i < reply->elements; i++, iterKeyValue++) 
		    {
			    redisReply* childReply = reply->element[i];
				SKeyValue &keyValue = *iterKeyValue;
				string &value = keyValue.strValue;
		        if (REDIS_REPLY_STRING == childReply->type)
		        {
		            value = string(childReply->str, childReply->len);  
		        }
				else if(REDIS_REPLY_INTEGER == reply->type)
				{
					char buf[33] = {0};
					snprintf(buf, sizeof(buf), "%lld", reply->integer);
					value = buf;
				}
				else if(REDIS_REPLY_NIL == reply->type)
				{
					value = "0";
				}
		    }
		}
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisStringAgent::%s, Server[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), reply->str);
	}

	freeReplyObject(reply);
	return returnNo;
}

int CRedisStringAgent::BatchIncrby(vector<SKeyValue> &vecKeyValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisStringAgent::%s\n", __FUNCTION__);
    string strFormat = "INCRBY %b ";
	
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
		if(REDIS_REPLY_STRING == reply->type)
		{
			keyValue.strValue = reply->str;
		}
		else if(REDIS_REPLY_INTEGER == reply->type)
		{
			char buf[33] = {0};
			snprintf(buf, sizeof(buf), "%lld", reply->integer);
			keyValue.strValue = buf;
		}
	}

	return RVS_SUCESS;
}

