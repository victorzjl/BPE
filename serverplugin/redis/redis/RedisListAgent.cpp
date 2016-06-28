#include "RedisListAgent.h"
#include "hiredis.h"
#include "RedisVirtualServiceLog.h"
#include "RedisVirtualService.h"
#include "ErrorMsg.h"
#include "WarnCondition.h"

using namespace redis;

int CRedisListAgent::Get(const string &strKey, string &strValue, const int index)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	string strFormat = "LINDEX %b ";
	char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", index);
	strFormat += szTemp;
	
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_STRING == reply->type)
	{
		strValue = reply->str;
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] index[%d] Value[%s] success\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), index, strValue.c_str());
	}
	else if(REDIS_REPLY_NIL == reply->type)
	{
	    returnNo = RVS_REDIS_LIST_INDEX_OUT_RANGE;
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] index[%d] out range\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), index);
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

int CRedisListAgent::Set(const string &strKey, const string &strValue, const int index)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	string strFormat = "LSET %b ";
	char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", index);
	strFormat += szTemp;
	strFormat += " %b";
	
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);
	
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_STATUS == reply->type && strcasecmp(reply->str, "OK") == 0)
	{
		returnNo = RVS_SUCESS;
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] index[%d] Value[%s] success\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), index, strValue.c_str());
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] index[%d] Value[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), index, strValue.c_str(), reply->str);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

int CRedisListAgent::Delete(const string &strKey, const string &strValue, const int count)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s] strValue[%s] count[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), count);
	string strFormat = "LREM %b ";
	char szTemp[32] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", count);
	strFormat += szTemp;
	strFormat += " %b";
		
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);
		
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
		
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER == reply->type)
	{
		returnNo = RVS_SUCESS;
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] count[%d] Value[%s] success Reply[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), count, strValue.c_str(), reply->integer);
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] count[%d] Value[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), count, strValue.c_str(), reply->str);
	}
			
	freeReplyObject(reply);
	return returnNo;
}

int CRedisListAgent::Size(const string &strKey, int &size)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	string strFormat = "LLEN %b";
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
		
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
		
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER != reply->type)
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
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
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] success, response[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->integer);
	}
			
	freeReplyObject(reply);
	return returnNo;
}

int CRedisListAgent::GetAll(const string &strKey, vector<string> &vecValue, const int start, const int end)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
	string strFormat = "LRANGE %b ";
	char szTemp[35] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d %d", start, end);
	strFormat += szTemp;
	
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
			
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
			
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s] start[%d] end[%d] ReplySize[%d]\n", __FUNCTION__, strKey.c_str(), start, end, reply->elements);
        if(reply->elements == 0)
        {
        	returnNo = RVS_REDIS_KEY_NOT_FOUND;
		    RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s] start[%d] end[%d] not found\n", __FUNCTION__, strKey.c_str(), start, end);
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
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
				
	freeReplyObject(reply);
	return returnNo;

}

int CRedisListAgent::CRedisListAgent::DeleteAll(const string &strKey)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
    return Del(strKey);
}

int CRedisListAgent::PopBack(const string &strKey, string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	string strFormat = "RPOP %b";
		
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
		
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
		
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_STRING == reply->type)
	{
		strValue = reply->str;
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] Value[%s] success\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str());
	}
	else if(REDIS_REPLY_NIL == reply->type)
	{
		returnNo = RVS_REDIS_KEY_NOT_FOUND;
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] not found\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str());
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
			
	freeReplyObject(reply);
	return returnNo;
}

int CRedisListAgent::PushBack(const string &strKey, const string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s] strValue[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());
	string strFormat = "RPUSH %b %b";
			
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);
			
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
			
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER != reply->type)
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] Value[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str(), reply->str);
	}
	else
	{
		WarnLength(reply->integer, m_pRedisAgent->GetAddr());
	}
				
	freeReplyObject(reply);
	return returnNo;
}

int CRedisListAgent::PopFront(const string &strKey, string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	string strFormat = "LPOP %b";
		
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
		
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
		
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_STRING == reply->type)
	{
		strValue = reply->str;
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] Value[%s] success\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str());
	}
	else if(REDIS_REPLY_NIL == reply->type)
	{
		returnNo = RVS_REDIS_KEY_NOT_FOUND;
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] not found\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str());
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
			
	freeReplyObject(reply);
	return returnNo;

}

int CRedisListAgent::PushFront(const string &strKey, const string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s] strValue[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());
	string strFormat = "LPUSH %b %b";
			
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);
			
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
			
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER != reply->type)
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] Value[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str(), reply->str);
	}
	else
	{
		WarnLength(reply->integer, m_pRedisAgent->GetAddr());
	}
				
	freeReplyObject(reply);
	return returnNo;

}

int CRedisListAgent::Reserve(const string &strKey, const int start, const int end)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, strKey[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
	string strFormat = "LTRIM %b ";
	char szTemp[35] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d %d", start, end);
	strFormat += szTemp;
	
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_STATUS == reply->type && strcasecmp(reply->str, "OK") == 0)
	{
		returnNo = RVS_SUCESS;
		RVS_XLOG(XLOG_DEBUG, "CRedisListAgent::%s, Server[%s] Key[%s] start[%d] end[%d] success\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), start, end);
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisListAgent::%s, Server[%s] Key[%s] start[%d] end[%d] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), start, end, reply->str);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

void CRedisListAgent::WarnLength(const unsigned int currentLength, const string &strAddr)
{
    if(m_pRedisVirtualService != NULL)
    {
        SDependencyWarnCondition objDependencyWarnCondition = m_pRedisVirtualService->GetWarnCondition()->GetDependencyWarnCondition();
	    unsigned int dwAlarmFrequency = objDependencyWarnCondition.dwAlarmFrequency;
		
		if(currentLength >= dwAlarmFrequency)
		{
			char szWarnInfo[256] = {0};
	        snprintf(szWarnInfo, sizeof(szWarnInfo), "Redis list length[%d] is over the limit[%d], Ip[%s]", currentLength, dwAlarmFrequency, strAddr.c_str());
	        m_pRedisVirtualService->Warn(szWarnInfo);
		}
    	
    }
	
}


