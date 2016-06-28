#include "RedisHashAgent.h"
#include "hiredis.h"
#include "RedisVirtualServiceLog.h"
#include "ErrorMsg.h"

CRedisHashAgent::~CRedisHashAgent()
{ 
}
/*
int CRedisHashAgent::Get(const string &strKey, map<string, string> &mapFieldValue)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
    int returnNo = RVS_SUCESS;
	bool found = false;
    string strCommand = "HMGET " + strKey;
	map<string, string>::iterator iter;
	for(iter = mapFieldValue.begin(); iter != mapFieldValue.end(); iter++)
	{
		strCommand += " ";
		strCommand += iter->first.c_str();
	}
	
	redisReply * reply = m_pRedisAgent->SendRedisCommand(strCommand);
	if(NULL == reply)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

	if (reply->type == REDIS_REPLY_ARRAY)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, ReplyStr[%s] ReplyLength[%d] ReplySize[%d]\n", __FUNCTION__, reply->str, reply->len, reply->elements);
        iter = mapFieldValue.begin();
		for(unsigned int i = 0; i < reply->elements; i++, iter++) 
		{
			redisReply* childReply = reply->element[i];
		    if (childReply->type == REDIS_REPLY_STRING)
		    {
		        RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, ChildReply[%s]\n", __FUNCTION__, childReply->str);
				found = true;
		    	mapFieldValue[iter->first] = childReply->str;
		    }
			else if(childReply->type == REDIS_REPLY_INTEGER)
			{
			    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, ChildReply[%lld]\n", __FUNCTION__, childReply->integer);
				found = true;
				char szPrefix[32] = {0};
	            snprintf(szPrefix, sizeof(szPrefix), "%lld", childReply->integer);
				mapFieldValue[iter->first] = szPrefix;
			}
		 }

		if(!found)
	    {
	        RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, Server[%s] : Get key not found, Key[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str());
		    returnNo = RVS_REDIS_KEY_NOT_FOUND;
	    }
    }
	else
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Get key error, Key[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
		returnNo = RVS_REDIS_CONNECT_ERROR;
	}

	freeReplyObject(reply);
	return returnNo;
	
}

//数据过期时间为秒
int CRedisHashAgent::SetAll(const string &strKey, const map<string, string> &mapFieldValue, unsigned int dwExpiration)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s], dwExpiration[%d]\n", __FUNCTION__, strKey.c_str(), dwExpiration);
    int returnNo = RVS_SUCESS;

	string arrayStrCommand[2];
	arrayStrCommand[0] = "HMSET " + strKey;
	map<string, string>::const_iterator iter;
	for(iter = mapFieldValue.begin(); iter != mapFieldValue.end(); iter++)
	{
		arrayStrCommand[0] += " ";
		arrayStrCommand[0] += iter->first.c_str();
		arrayStrCommand[0] += " "; 
		arrayStrCommand[0] += iter->second.c_str();
	}
	
	if(dwExpiration > 0)
	{
		char tempStr[32] = {0};
	    snprintf(tempStr, sizeof(tempStr), " %d", dwExpiration);
		arrayStrCommand[1] = "EXPIRE " + strKey + tempStr;
	}
	
	int nRet = m_pRedisAgent->AppendPipelineCommand(arrayStrCommand, 2);
	if(nRet != 0)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    //解析域设置响应
    redisReply * reply = NULL;
	nRet = m_pRedisAgent->GetReplyInPipeline(&reply);
	if(NULL == reply)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Command Faild[%s], Reply is NULL, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), arrayStrCommand[0].c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if (reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str,"OK") == 0) 
	{
		returnNo = RVS_SUCESS;
	}
	else
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Command[%s] Faild, Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), arrayStrCommand[0].c_str(), reply->str);
		returnNo = RVS_REDIS_CONNECT_ERROR;
	}
	freeReplyObject(reply);

    if(dwExpiration > 0)
    {
        //解析超时时间设置的响应
        reply = NULL;
    	nRet = m_pRedisAgent->GetReplyInPipeline(&reply);
	    if(NULL == reply)
	    {
	        RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Command Faild[%s], ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), arrayStrCommand[0].c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		    return RVS_REDIS_CONNECT_ERROR;
	    }
	
	    if (reply->type == REDIS_REPLY_INTEGER) 
	    {
	        if(reply->integer == 1)
	        {
	            RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, Server[%s] : Command[%s] Success\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), arrayStrCommand[1].c_str());
	        }
			else if(reply->integer == 0)
			{
				RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Command Faild[%s], Error[ Key[%s] no exist or Expire time can't be set ]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), arrayStrCommand[1].c_str() , strKey.c_str());
			}	    
	    }
	    else
	    {
		    RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Command[%s] Faild, Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), arrayStrCommand[1].c_str(), reply->str);
	    }
	    freeReplyObject(reply);
    }
	
	return returnNo;

}
*/

int CRedisHashAgent::Get(const string &strKey, map<string, string> &mapFieldValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s\n", __FUNCTION__);
    int returnNo = RVS_SUCESS;

    vector<string> vecCmdArgs;
	vecCmdArgs.push_back("HMGET");
	vecCmdArgs.push_back(strKey);
    map<string, string>::iterator iter = mapFieldValue.begin();
	for(;iter != mapFieldValue.end(); iter++)
	{
		vecCmdArgs.push_back(iter->first);
	}

	if(0 != m_pRedisAgent->AppendCmdArgv(vecCmdArgs))
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    redisReply *reply = NULL;
    if(0 != m_pRedisAgent->GetReplyInPipeline(&reply))
    {
   	    return RVS_REDIS_CONNECT_ERROR;
    }

	if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, Server[%s] : Size[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), reply->elements);
        if(reply->elements == 0)
        {
        	returnNo = RVS_REDIS_KEY_NOT_FOUND;
		    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, Server[%s] : Get key not found\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str());
        }
		else
		{
		    bool hasResult = false;
			iter = mapFieldValue.begin();
		    for(unsigned int i = 0; i < reply->elements; i++, iter++) 
		    {
			    redisReply* childReply = reply->element[i];
				string &value = iter->second;
		        if (REDIS_REPLY_STRING == childReply->type)
		        {
		            hasResult = true;
		            value = string(childReply->str, childReply->len);  
		        }
				else if(REDIS_REPLY_INTEGER == reply->type)
				{
				    hasResult = true;
					char buf[33] = {0};
					snprintf(buf, sizeof(buf), "%lld", reply->integer);
					value = buf;
				}
				else if(REDIS_REPLY_NIL == reply->type)
				{
					value = "0";
				}
		    }

			if(!hasResult)
			{
				returnNo = RVS_REDIS_KEY_NOT_FOUND;
			}
		}
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), reply->str);
	}

	freeReplyObject(reply);
	return returnNo;
}

int CRedisHashAgent::GetBinaryOperation(const string &strKey,const string &strField, string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s] strField[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());
	
	redisContext * r = m_pRedisAgent->GetRedisContext();
	if(r == NULL)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return RVS_REDIS_CONNECT_ERROR;
	}
		
	int returnNo = RVS_SUCESS;
	string strFormat = "HGET %b %b";
	redisReply * reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strField);
		
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
			
	if (reply->type == REDIS_REPLY_STRING)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, ReplyStr[%s] ReplyLength[%d]\n", __FUNCTION__, reply->str, reply->len);
	    strValue = string(reply->str, reply->len);  
    }
	else if(reply->type == REDIS_REPLY_NIL)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, Server[%s] : Redis get key not found, Key[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str());
		returnNo = RVS_REDIS_KEY_NOT_FOUND;
	}
	else
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis get key error, Key[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}
			
	freeReplyObject(reply);
	return returnNo;
}

int CRedisHashAgent::Set(const string &strKey, const map<string, string> &mapFieldValue, unsigned int dwExpiration)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s], dwExpiration[%d]\n", __FUNCTION__, strKey.c_str(), dwExpiration);
	int returnNo = RVS_SUCESS;

	//开始事务
    returnNo = m_pRedisAgent->BeginTransaction();
	if(returnNo != RVS_SUCESS)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s failed ErrorNo[%d]\n", __FUNCTION__, returnNo);
		return returnNo;
	}
		
	map<string, string>::const_iterator iter;
	for(iter = mapFieldValue.begin(); iter != mapFieldValue.end(); iter++)
	{
		returnNo = SaveBinaryData(strKey, iter->first, iter->second);
		if(returnNo != RVS_SUCESS)
		{
			return returnNo;
		}
	}
	if(dwExpiration > 0)
	{
		ExpireInTransaction(strKey, dwExpiration);
	}

    //结束事务并分析执行结果
	redisReply * reply = NULL;
	reply = m_pRedisAgent->EndTransaction();
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_ERROR == reply->type)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] strKey[%s] ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
		returnNo = RVS_SYSTEM_ERROR;
	}
	else if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s] ReplySize[%d] sucess\n", __FUNCTION__, strKey.c_str(), reply->elements);
		returnNo = RVS_SUCESS;
	}
	
	freeReplyObject(reply);
	return returnNo;
}

int CRedisHashAgent::SaveBinaryData(const string &strKey, const string &strField, const string &strValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s], strField[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());

	redisContext * r = m_pRedisAgent->GetRedisContext();
	if(r == NULL)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, RedisServer is not initialized\n", __FUNCTION__);
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	string strFormat = "HSET %b %b %b";
	redisReply * reply = NULL;
	reply = (redisReply *)redisCommand(r, strFormat.c_str(), strKey.c_str(), strKey.length(), 
		strField.c_str(), strField.length(),
		strValue.c_str(), strValue.length());
	
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

    if(REDIS_REPLY_ERROR == reply->type)
    {
    	RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] strKey[%s] strField[%s] strValue[%s] ReplyError[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), strValue.c_str(), reply->str);
    }
	freeReplyObject(reply);
	return RVS_SUCESS;
}

int CRedisHashAgent::Delete(const string &strKey, const vector<string> &vecField)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	int returnNo = RVS_SUCESS;

	vector<string>::const_iterator iter;
	for(iter = vecField.begin(); iter != vecField.end(); iter++)
	{
		returnNo = DeleteBinaryOperation(strKey, *iter);
		if(returnNo == RVS_REDIS_CONNECT_ERROR)
		{
			return returnNo;
		}
	}
	return RVS_SUCESS;
}

int CRedisHashAgent::DeleteBinaryOperation(const string &strKey,const string &strField)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s], strField[%s]\n", __FUNCTION__, strKey.c_str(), strField.c_str());
	int returnNo = RVS_SUCESS;
    string strFormat = "HDEL %b %b";
	redisReply * reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strField);
	if(NULL == reply)
	{
	    RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}

	if(REDIS_REPLY_INTEGER != reply->type)
	{
	    returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Key[%s] Field[%s], Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), reply->str);
    }
	else
	{
		RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, Server[%s] : Key[%s] Field[%s] success, response[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), reply->integer);
	}
	
	freeReplyObject(reply);
	return returnNo;
}

int CRedisHashAgent::Size(const string &strKey, int &size)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	int returnNo = RVS_SUCESS;
	
	string strFormat = "HLEN %b";
	
	redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_INTEGER == reply->type)
	{
        size = reply->integer;
		if(size == 0)
		{
			returnNo = RVS_REDIS_KEY_NOT_FOUND;
		}
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Key[%s] Faild, Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

int CRedisHashAgent::GetAll(const string &strKey, map<string, string> &mapFieldValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
	int returnNo = RVS_SUCESS;
		
	string strFormat = "HGETALL %b";
    redisReply * reply = m_pRedisAgent->BinaryOperationByOneParam(strFormat, strKey);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
	
	if(REDIS_REPLY_ARRAY == reply->type)
	{
	    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, Server[%s] : Key[%s], Size[%d]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->elements);
        if(reply->elements == 0)
        {
        	returnNo = RVS_REDIS_KEY_NOT_FOUND;
		    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, Server[%s] : Get key not found, Key[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str());
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
		        	   mapFieldValue[temp] = childReply->str;
		            }
		        }
		    }
		}
	}
	else
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Key[%s] Faild, Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), reply->str);
	}
		
	freeReplyObject(reply);
	return returnNo;
}

int CRedisHashAgent::DeleteAll(const string &strKey)
{
    RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s]\n", __FUNCTION__, strKey.c_str());
    return Del(strKey);
}

int CRedisHashAgent::Incby(const string &strKey, const string &strField, const int increment)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s, strKey[%s], strField[%s], increment[%d]\n", __FUNCTION__, strKey.c_str(), strField.c_str(), increment);
	int returnNo = RVS_SUCESS;
		
	string strFormat = "HINCRBY %b %b ";
	char szTemp[16] = {0};
	snprintf(szTemp, sizeof(szTemp), "%d", increment);
	strFormat += szTemp;
		
	redisReply * reply = m_pRedisAgent->BinaryOperationByTwoParam(strFormat, strKey, strField);
	if(NULL == reply)
	{
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Redis connect error, ErrorNo[%d], ErrorStr[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), m_pRedisAgent->GetError(), m_pRedisAgent->GetErrorStr());
		return RVS_REDIS_CONNECT_ERROR;
	}
		
	if(REDIS_REPLY_INTEGER != reply->type)
	{
		returnNo = RVS_SYSTEM_ERROR;
		RVS_XLOG(XLOG_ERROR, "CRedisHashAgent::%s, Server[%s] : Key[%s] Field[%s] Increment[%d] Faild, Error[%s]\n", __FUNCTION__, m_pRedisAgent->GetAddr().c_str(), strKey.c_str(), strField.c_str(), increment, reply->str);
	}
			
	freeReplyObject(reply);
	return returnNo;
}

int CRedisHashAgent::BatchQuery(vector<SKeyFieldValue> &vecKFV)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s\n", __FUNCTION__);
    string strFormat = "HGET %b %b";
	
    vector<SKeyFieldValue>::iterator iter = vecKFV.begin();
	int waitInPipe = 0;
	for(;iter != vecKFV.end(); iter++)
	{
		SKeyFieldValue &keyFieldValue = *iter;
		int result = m_pRedisAgent->AppendCmdByTwoParam(strFormat, keyFieldValue.strKey, keyFieldValue.strField);
	     if(-1 == result)
	     {
	     	return RVS_REDIS_CONNECT_ERROR;
	     }
		 else if(result == 0)
		 {
		 	waitInPipe++;
		 }
	}

    iter = vecKFV.begin();
	for(int i = 0;i < waitInPipe && iter != vecKFV.end(); i++, iter++)
	{
		redisReply *reply = NULL;
		if(0 != m_pRedisAgent->GetReplyInPipeline(&reply))
		{
			return RVS_REDIS_CONNECT_ERROR;
		}
		SKeyFieldValue &keyFieldValue = *iter;
		if(REDIS_REPLY_STRING == reply->type)
		{
			keyFieldValue.strValue = string(reply->str, reply->len);
		}
		else if(REDIS_REPLY_INTEGER == reply->type)
		{
			char buf[33] = {0};
			snprintf(buf, sizeof(buf), "%lld", reply->integer);
			keyFieldValue.strValue = buf;
		}
		else if(REDIS_REPLY_NIL == reply->type)
		{
			keyFieldValue.strValue = "0";
		}
	}

	return RVS_SUCESS;
}

int CRedisHashAgent::BatchIncrby(vector<SKeyFieldValue> &vecKFV)
{
	RVS_XLOG(XLOG_DEBUG, "CRedisHashAgent::%s\n", __FUNCTION__);
    string strFormat = "HINCRBY %b %b ";
	
    vector<SKeyFieldValue>::iterator iter = vecKFV.begin();
	int waitInPipe = 0 ;
	for(;iter != vecKFV.end(); iter++)
	{
		SKeyFieldValue &keyFieldValue = *iter;
		string finalFormat = strFormat + keyFieldValue.strValue;
		int result = m_pRedisAgent->AppendCmdByTwoParam(finalFormat, keyFieldValue.strKey, keyFieldValue.strField);
	     if(-1 == result)
	     {
	     	return RVS_REDIS_CONNECT_ERROR;
	     }
		 else if(result == 0)
		 {
		 	waitInPipe++;
		 }
	}

    iter = vecKFV.begin();
	for(int i = 0;i < waitInPipe && iter != vecKFV.end(); i++, iter++)
	{
		redisReply *reply = NULL;
		if(0 != m_pRedisAgent->GetReplyInPipeline(&reply))
		{
			return RVS_REDIS_CONNECT_ERROR;
		}
		SKeyFieldValue &keyFieldValue = *iter;
		if(REDIS_REPLY_INTEGER == reply->type)
		{
			char buf[33] = {0};
			snprintf(buf, sizeof(buf), "%lld", reply->integer);
			keyFieldValue.strValue = buf;
		}
	}

	return RVS_SUCESS;
}

