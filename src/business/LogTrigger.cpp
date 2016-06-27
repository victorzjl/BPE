#include "LogTrigger.h"
#include <arpa/inet.h>
#include "AvenueServiceConfigs.h"
#include "AvenueMsgHandler.h"
#include "SapMessage.h"
#include "SapLogHelper.h"
#include "ComposeService.h"
#include "ComposeServiceObj.h"

const char* ASC_VERSION = "2.3.8.1";

string GetDefaultValue(int nNum);
void  ReplaceSpace(string& oString);

string BuildLog(CAvenueMsgHandler *pHandler, const map<string, vector<string> >* mapDefValue, const vecTriggerItem& vecItem,
                map<int, string>& mapKey);

string GetProcValue(const string& strKey, const map<string, SDefParameter>& mapDefaultValue, map<string, SDefValue>& mapValue);

void BuildLogInfo(SLogInfo &sLog, const string &strRes)
{
    char szBuf[MAX_LOG_BUFFER];
    snprintf(szBuf, sizeof(szBuf)-1, 
        "%s,  %s,  %s,  %s,  %s,  %s", 
        ASC_VERSION, sLog.mapKey[1].c_str(), sLog.mapKey[2].c_str(), sLog.mapKey[3].c_str(),
        sLog.strReq[0].c_str(), strRes.c_str());
    sLog.logInfo = szBuf;

    string strOtherKey;
    bool muti = false;
    map<int, string>::const_iterator itr = sLog.mapKey.begin();
    for (; itr != sLog.mapKey.end(); ++itr)
    {
        if (itr->first > 6)
        {
            if (muti)
            {
                strOtherKey.append("^_^");
            }
            strOtherKey.append(itr->second);
            muti = true;
        }
    }

    snprintf(szBuf, sizeof(szBuf)-1, 
        "%s,  %s,  %s,  %s", 
        sLog.mapKey[4].c_str(), sLog.mapKey[5].c_str(), sLog.mapKey[6].c_str(), strOtherKey.c_str());

    sLog.logInfo2 = szBuf;
}

const char SPLIT_STR[] = "^_^";
const char* LOG_SPLIT_STR = ",  ";


void CLogTrigger::BeginTrigger(const void *pBuffer, int nLen, SLogInfo &sLog)
{    
    SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;
    int nServId = ntohl(pHead->dwServiceId);
    int nMsgId  = ntohl(pHead->dwMsgId);       
    
    sLog.pItem = CTriggerConfig::Instance()->GetInfo(TRIGGERTYPE,nServId, nMsgId);    
    if (sLog.pItem == NULL || sLog.pItem->nReqSize == 0)
    {
        return;
    }  
    
    void *pBody  = (unsigned char *)pBuffer + pHead->byHeadLen;
    int nBodyLen = nLen - pHead->byHeadLen;  
    if (nBodyLen == 0)
    {
        sLog.strReq[0] = GetDefaultValue(sLog.pItem->nReqSize);
        return;
    }
    
    if (m_pConfig == NULL || 
        0 != m_pConfig->GetServiceNameById(nServId, nMsgId, sLog.serviceName))
    {  
        SS_XLOG(XLOG_DEBUG,"CLogTrigger::%s service[%d],msg[%d] not found\n",
            __FUNCTION__,nServId, nMsgId); 
        sLog.strReq[0] = GetDefaultValue(sLog.pItem->nReqSize);
        return;
    }
    
    CAvenueMsgHandler oHandler(sLog.serviceName,true,m_pConfig);      
    oHandler.Decode(pBody, nBodyLen);
    sLog.strReq[0] = BuildLog(&oHandler, NULL, sLog.pItem->vecReq, sLog.mapKey); 
}


void CLogTrigger::EndTrigger(const void *pBuffer, int nLen, SLogInfo &sLog)
{           
    const STriggerItem *pItem = sLog.pItem;    
    if (pItem->nResSize == 0 )
    {        
        BuildLogInfo(sLog, "NULL");
        return;
    }        
    string strRes;
    if (pBuffer == NULL)
    {
        strRes = GetDefaultValue(pItem->nResSize); 
    }
    else
    {    
        SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;
        void *pBody = (unsigned char *)pBuffer + pHead->byHeadLen;
        int nBodyLen = nLen - pHead->byHeadLen;          
        if (/* nBodyLen > 0 &&*/ m_pConfig != NULL)
        {        
            CAvenueMsgHandler oHandler(sLog.serviceName, false, m_pConfig);      
            oHandler.Decode(pBody, nBodyLen);
            strRes = BuildLog(&oHandler, NULL, pItem->vecRes, sLog.mapKey);        
        }
        else
        {  
            strRes = GetDefaultValue(pItem->nResSize); 
        }  
    }
    BuildLogInfo(sLog, strRes);
}


void CLogTrigger::BeginTrigger(CAvenueMsgHandler *pHandler, int nServId, int nMsgId, SLogInfo& sInfo)
{    
    sInfo.pItem = CTriggerConfig::Instance()->GetInfo(TRIGGERTYPE,nServId, nMsgId);
    if (sInfo.pItem != NULL && sInfo.pItem->nReqSize > 0)
    {
        if (nServId == BNB_SERVICEID)
        {
            sInfo.isBNB = true;
            vecTriggerItem user1(sInfo.pItem->vecReq.begin(), sInfo.pItem->vecReq.begin()+sInfo.pItem->nReqSize/2);
            vecTriggerItem user2(sInfo.pItem->vecReq.begin()+sInfo.pItem->nReqSize/2, sInfo.pItem->vecReq.end());
            sInfo.strReq[0] = BuildLog(pHandler, NULL, user1, sInfo.mapKey);
            sInfo.strReq[1] = BuildLog(pHandler, NULL, user2, sInfo.mapKey2);
        }
        else
        {
            sInfo.strReq[0] = BuildLog(pHandler, NULL, sInfo.pItem->vecReq, sInfo.mapKey);
        }
    }
    SS_XLOG(XLOG_DEBUG,"CLogTrigger::%s service[%d],msg[%d] value[%s]\n",
            __FUNCTION__,nServId, nMsgId,sInfo.strReq[0].c_str());
}

void CLogTrigger::BeginDataTrigger(SLogInfo& sInfo, CAvenueMsgHandler *pReqHandler, CAvenueMsgHandler *pRspHandler, int nServId, int nMsgId, const map<string, vector<string> >* mapDefValue)
{    
    sInfo.pItem = CTriggerConfig::Instance()->GetInfo(TRIGGERTYPE, nServId, nMsgId);
    if (sInfo.pItem != NULL)
    {
        string strReq = BuildLog(pReqHandler, NULL, sInfo.pItem->vecReq, sInfo.mapKey);
        string strRsp = BuildLog(pRspHandler, mapDefValue, sInfo.pItem->vecRes, sInfo.mapKey);
        sInfo.strReq[0] = strReq + "@_@" + strRsp;
    }
    SS_XLOG(XLOG_DEBUG,"CLogTrigger::%s service[%d],msg[%d] value[%s]\n",
        __FUNCTION__,nServId, nMsgId,sInfo.strReq[0].c_str());
}

void CLogTrigger::EndTrigger(const void *pBody,
                             int nLen,
                             CAvenueMsgHandler *pHandler,
                             SLogInfo &sLog,
                             const map<string, vector<string> >* mapDefValue)
{
    if (sLog.isBNB)
    {
        if (sLog.pItem->nResSize == 0 )
        {        
            BuildLogInfo(sLog, "NULL");
            return;
        }
        string strRes[2];
        if (nLen > 0)
        {        
            pHandler->Decode(pBody, nLen);     
            vecTriggerItem user1(sLog.pItem->vecRes.begin(), sLog.pItem->vecRes.begin()+sLog.pItem->nResSize/2);
            vecTriggerItem user2(sLog.pItem->vecRes.begin()+sLog.pItem->nResSize/2, sLog.pItem->vecRes.end());
            strRes[0] = BuildLog(pHandler, mapDefValue, user1, sLog.mapKey);
            strRes[1] = BuildLog(pHandler, mapDefValue, user2, sLog.mapKey2);
        }
        else
        {        
            strRes[0] = GetDefaultValue(sLog.pItem->nResSize);
            strRes[1] = strRes[0];
        }
        BuildLogInfo(sLog, strRes[0]);
        char szBuf[MAX_LOG_BUFFER];
        snprintf(szBuf, sizeof(szBuf)-1, 
            "%s,  %s,  %s,  %s,  %s,  %s", 
            ASC_VERSION, sLog.mapKey2[1].c_str(), sLog.mapKey2[2].c_str(), sLog.mapKey2[3].c_str(),
            sLog.strReq[1].c_str(), strRes[1].c_str());
        sLog.plusLogInfo = szBuf;
    }
    else
    {
        string strRes;

        if (nLen > 0)
        {        
			SS_XLOG(XLOG_DEBUG,"CLogTrigger::%s xxx\n",__FUNCTION__);
            pHandler->Decode(pBody, nLen);  
			if (pHandler!=NULL)
			{
				strRes = BuildLog(pHandler, mapDefValue, sLog.pItem->vecRes, sLog.mapKey);
				SS_XLOG(XLOG_DEBUG,"CLogTrigger::%s %d value[%s]\n",__FUNCTION__,sLog.pItem->vecRes.size(),strRes.c_str());
			}			
        }

        BuildLogInfo(sLog, strRes);
    }
}

void CLogTrigger::EndDataTrigger(SLogInfo &sLog, const map<string, vector<string> >* mapDefValue)
{
    string strDataDef = BuildLog(NULL, mapDefValue, sLog.pItem->vecDataDef, sLog.mapKey);

    BuildLogInfo(sLog, strDataDef);

}

string CLogTrigger::GetDefaultLog(int nServId, int nMsgId)
{
    STriggerItem *pItem = CTriggerConfig::Instance()->GetInfo(TRIGGERTYPE,nServId, nMsgId);
    const string defReq = GetDefaultValue(pItem->nReqSize);
    const string defRes = GetDefaultValue(pItem->nResSize);
    char szBuf[MAX_LOG_BUFFER];
    snprintf(szBuf, sizeof(szBuf)-1, "%s,  ,  ,  %s,  %s", 
        ASC_VERSION, defReq.c_str(), defRes.c_str());    
    return szBuf;
}

string BuildLog(CAvenueMsgHandler *pHandler, const map<string, vector<string> >* mapDefValue,
                const vecTriggerItem& vecItem, map<int, string>& mapKey)
{
    string value, item;
    for (vecTriggerItem::const_iterator itr = vecItem.begin(); 
        itr != vecItem.end();)
    {            
        const STriggerInfo& sInfo = *itr; 

        item = sInfo.pFunc(sInfo.strName, pHandler, SLogPara(sInfo.strDefault,sInfo.nKey), mapDefValue);

		ReplaceSpace(item);
        value.append(item);
        if (sInfo.keyType > 0 && !item.empty())
        {
            string& mapitem = mapKey[sInfo.keyType];
            if (!item.empty())
            {
                mapitem = item;
            }
        }
        if (++itr != vecItem.end())
        {
            value.append(SPLIT_STR);  
        }
    }
    return value;
}

inline string GetDefaultValue(int nNum)
{   
    if (nNum == 0)
    {
        return string("NULL");
    }
    string strValue; 
    for (int i = 0; i < nNum-1; ++i)
    {        
        strValue.append(SPLIT_STR);        
    }        
    return strValue;
}

void  ReplaceSpace(string& oString)
{
	//",  " => ", "
	string& tempStr = oString;
    string::size_type pos = tempStr.find(",  ");
    while (pos != string::npos)
    {
        tempStr.erase(pos+2, 1);
        pos = tempStr.find(",  ",pos);
    }
}
