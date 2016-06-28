#include "RedisZsetAgent.h"
#include "hiredis.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"

int CRedisZsetAgent::Get(const string &strKey, const string &strValue, int &score)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strValue[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());
    int returnNo = RVS_SUCESS;
    string strFormat = "ZSCORE %b %b";
	
	redisReply * reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);
	if(NULL == reply)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

	if(reply->type == REDIS_REPLY_STRING)
	{
	    string strScore = reply->str;
		score = atoi(strScore.c_str());
	}
	else if(reply->type == REDIS_REPLY_NIL)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strValue[%s] not found\n", __FUNCTION__, strKey.c_str(), strValue.c_str());
		returnNo = RVS_REDIS_KEY_NOT_FOUND;
	}
	else
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] Key[%s] Value[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}

	freeReplyObject(reply);
	return returnNo;

}

int CRedisZsetAgent::Set(const string &strKey, const string &strValue, const int score, unsigned int dwExpiration, int &result)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strValue[%s] score[%d] dwExpiration[%d]\n", __FUNCTION__, strKey.c_str(), strValue.c_str(), score, dwExpiration);
    int returnNo = RVS_SUCESS;

	//开始事务
    returnNo = m_pRedisAgent->BeginTransaction();
	if(returnNo != RVS_SUCESS)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s failed ErrorNo[%d]\n", __FUNCTION__, returnNo);
		return returnNo;
	}
	
	returnNo = SaveBinaryDataInTransaction(strKey, strValue, score);
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
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_ERROR == reply->type)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] strKey[%s] ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}
	else if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s] ReplySize[%d] sucess\n", __FUNCTION__, strKey.c_str(), reply->elements);
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

int CRedisZsetAgent::SaveBinaryDataInTransaction(const string &strKey,const string &strValue, const int score)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strValue[%s]\n", __FUNCTION__, strKey.c_str(), strValue.c_str());
		
	char szTemp[16] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", score);
    string strFormat = "ZADD %b ";
	strFormat += szTemp;
	strFormat += " %b";
	redisReply *reply = NULL;
	
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strValue);
		
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_ERROR == reply->type)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] strKey[%s] strValue[%s] score[%d] ReplyError[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strValue.c_str(), score, reply->str);
	}
	freeReplyObject(reply);
	return RVS_SUCESS;
}


int CRedisZsetAgent::Delete(const string &strKey, const string &strField)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strField[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());
    string strFormat = "ZREM %b %b";
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strField);

	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER != reply->type)
	{
	    returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] Key[%s] Field[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), reply->str);
    }
	else
	{
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] Field[%s] success, response[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), reply->integer);
	}
	
	freeReplyObject(reply);
	return returnNo;
}

int CRedisZsetAgent::Size(const string &strKey, int &size)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	string strFormat = "ZCARD %b";
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER != reply->type)
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] Key[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
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
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] success, response[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->integer);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

int CRedisZsetAgent::GetAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
	int returnNo = RVS_SUCESS;
	char szTemp[34] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d %d", start, end);
	string strFormat = "ZRANGE %b ";
	strFormat += szTemp;
	strFormat += " WITHSCORES";

    redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] : Key[%s], Size[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->elements);
        if(reply->elements == 0)
        {
        	returnNo = RVS_REDIS_ZSET_NO_MEMBERS;
		    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] no members\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str());
        }
		else
		{
			string temp;
		    for(unsigned int i = 0; i < reply->elements; i++) 
		    {
			    redisReply* childReply = reply->element[i];
		        if (childReply->type == REDIS_REPLY_STRING)
		        {
		            if(i % 2 == 0)
		            {
		        	   temp = childReply->str;
		            }
		            else
		            {
		        	   mapFieldAndScore[temp] = childReply->str;
		            }
		        }
		    }
		}
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Key[%s] Faild, Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

int CRedisZsetAgent::GetReverseAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
	int returnNo = RVS_SUCESS;

	char szTemp[34] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d %d", start, end);
	string strFormat = "ZREVRANGE %b ";
	strFormat += szTemp;
	strFormat += " WITHSCORES";
	
    redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] : Key[%s], Size[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->elements);
        if(reply->elements == 0)
        {
        	returnNo = RVS_REDIS_ZSET_NO_MEMBERS;
		    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] no members\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str());
        }
		else
		{
			string temp;
		    for(unsigned int i = 0; i < reply->elements; i++) 
		    {
			    redisReply* childReply = reply->element[i];
		        if (childReply->type == REDIS_REPLY_STRING)
		        {
		            if(i % 2 == 0)
		            {
		        	   temp = childReply->str;
		            }
		            else
		            {
		        	   mapFieldAndScore[temp] = childReply->str;
		            }
		        }
		    }
		}
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Key[%s] Faild, Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

int CRedisZsetAgent::GetScoreAll(const string &strKey, const int start, const int end, map<string, string> &mapFieldAndScore, bool bInputStart, bool bInputEnd)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
	int returnNo = RVS_SUCESS;
	string strStart;
	if(bInputStart)
	{
	    char szStartTemp[16] = {0};
		snprintf(szStartTemp, sizeof(szStartTemp), "%d", start);
		strStart = szStartTemp;
	}
    else
	{
	    strStart = "-inf";
	}
	string strEnd;
	if(bInputEnd)
	{
	    char szEndTemp[16] = {0};
		snprintf(szEndTemp, sizeof(szEndTemp), "%d", end);
		strEnd = szEndTemp;
	}
    else
	{
	    strEnd = "+inf";
	}
	string strFormat = "ZRANGEBYSCORE %b ";
	strFormat += strStart + " " + strEnd;
	strFormat += " WITHSCORES";
	
    redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] : Key[%s] start[%d] end[%d] Size[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), start, end, reply->elements);
        if(reply->elements == 0)
        {
        	returnNo = RVS_REDIS_ZSET_NO_MEMBERS;
		    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] start[%d] end[%d] no members\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), start, end);
        }
		else
		{
			string temp;
		    for(unsigned int i = 0; i < reply->elements; i++) 
		    {
			    redisReply* childReply = reply->element[i];
		        if (childReply->type == REDIS_REPLY_STRING)
		        {
		            if(i % 2 == 0)
		            {
		        	   temp = childReply->str;
		            }
		            else
		            {
		        	   mapFieldAndScore[temp] = childReply->str;
		            }
		        }
		    }
		}
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Key[%s] start[%d] end[%d] Faild, Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), start, end, reply->str);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

int CRedisZsetAgent::Rank(const string &strKey, const string &strField, int &rank)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strField[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());
    string strFormat = "ZRANK %b %b";
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strField);

	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] Field[%s] success, response[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), reply->integer);
		rank = reply->integer;
    }
	else if(REDIS_REPLY_NIL== reply->type)
	{
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strField[%s] not found\n", __FUNCTION__, strKey.c_str(), strField.c_str());
		returnNo = RVS_REDIS_KEY_NOT_FOUND;
	}
	else
	{
	    returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] Key[%s] Field[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), reply->str);
	}	
	
	freeReplyObject(reply);
	return returnNo;
}

int CRedisZsetAgent::ReverseRank(const string &strKey, const string &strField, int &rank)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strField[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());
    string strFormat = "ZREVRANK %b %b";
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strField);

	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] Field[%s] success, response[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), reply->integer);
		rank = reply->integer;
    }
	else if(REDIS_REPLY_NIL== reply->type)
	{
		RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strField[%s] not found\n", __FUNCTION__, strKey.c_str(), strField.c_str());
		returnNo = RVS_REDIS_KEY_NOT_FOUND;
	}
	else
	{
	    returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] Key[%s] Field[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), reply->str);
	}	
	
	freeReplyObject(reply);
	return returnNo;
}

int CRedisZsetAgent::Incby(const string &strKey, const string &strField, const int score)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s], strField[%s] score[%d]\n", __FUNCTION__, strKey.c_str(), strField.c_str(), score);
    char szTemp[16] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", score);
	string strFormat = "ZINCRBY %b ";
    strFormat += szTemp;
	strFormat += " %b";
	
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strField);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_STRING == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] Field[%s] score[%d] success, response[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), score, reply->str);
    }
	else
	{
	    returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] Key[%s] Field[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), reply->str);
	}	
	
	freeReplyObject(reply);
	return returnNo;
}

int CRedisZsetAgent::DelRangeByRank(const string &strKey, const int start, const int end)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
    char szTemp[34] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d %d", start, end);
	string strFormat = "ZREMRANGEBYRANK %b ";
	strFormat += szTemp;
	
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] start[%d] end[%d] success, DelSize[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), start, end, reply->integer);
    }
	else
	{
	    returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] Key[%s] start[%d] end[%d] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), start, end, reply->str);
	}	
	
	freeReplyObject(reply);
	return returnNo;
}

int CRedisZsetAgent::DelRangeByScore(const string &strKey, const int start, const int end, bool bInputStart, bool bInputEnd)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, strKey[%s] start[%d] end[%d]\n", __FUNCTION__, strKey.c_str(), start, end);
    string strStart;
	if(bInputStart)
	{
	    char szStartTemp[16] = {0};
		snprintf(szStartTemp, sizeof(szStartTemp), "%d", start);
		strStart = szStartTemp;
	}
    else
	{
	    strStart = "-inf";
	}
	string strEnd;
	if(bInputEnd)
	{
	    char szEndTemp[16] = {0};
		snprintf(szEndTemp, sizeof(szEndTemp), "%d", end);
		strEnd = szEndTemp;
	}
    else
	{
	    strEnd = "+inf";
	}
	
	string strFormat = "ZREMRANGEBYSCORE %b ";
	strFormat += strStart + " " + strEnd;
	
	redisReply *reply = NULL;
	reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    int returnNo = RVS_SUCESS;
	if(REDIS_REPLY_INTEGER == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisZsetAgent::%s, Server[%s] Key[%s] start[%d] end[%d] success, DelSize[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), start, end, reply->integer);
    }
	else
	{
	    returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisZsetAgent::%s, Server[%s] Key[%s] start[%d] end[%d] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), start, end, reply->str);
	}	
	
	freeReplyObject(reply);
	return returnNo;
}

