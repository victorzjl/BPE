#include "TriggerConfig.h"
#include <string>
#include <arpa/inet.h>
#include "tinyxml/tinyxml.h"
//#include "tinyxml/ex_tinyxml.h"
#include "AvenueMsgHandler.h"
#include "SapLogHelper.h"

using std::string;
using std::make_pair;

//using namespace sdo::util;
using sdo::commonsdk::SStructValue;

string GetIntValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved = NULL);
string GetKeyValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved = NULL);

string GetStringValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved = NULL);
string GetStringArray(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved = NULL);
string GetIntArray(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved = NULL);
string GetKeyValueArray(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved = NULL);
string GetPassword(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved = NULL);

string GetIntDefValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pDefValues);
string GetStringDefValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pDefValues);

int Remove0A0D(char* pAccount, int nLen);

CTriggerConfig * CTriggerConfig::m_instance = NULL;
CTriggerConfig* CTriggerConfig::Instance()
{
    if (m_instance == NULL)
    {
        m_instance = new CTriggerConfig;
    }
    return m_instance;
}

STriggerItem* CTriggerConfig::GetInfo(int nType,int nServId, int nMsgId)
{
    mapTrigger::iterator itr = m_mapTrigger.find(nType);
    if (itr != m_mapTrigger.end())
    {
        STriggerType & sType = itr->second;  
        mapTriggerInfo::iterator itrInfo = sType.mapInfo.find(SComposeKey(nServId,nMsgId));
        if (itrInfo != sType.mapInfo.end())
        {              
            return &itrInfo->second;
        }        
        return &sType.defaultItem;        
    }
    return NULL;
}

int CTriggerConfig::LoadConfig(const string &fileName)
{    
    SS_XLOG(XLOG_DEBUG,"CTriggerConfig::LoadConfig\n");
    TiXmlDocument xmlDoc;
	xmlDoc.LoadFile(fileName);
	if (xmlDoc.Error())
	{
		SS_XLOG(XLOG_WARNING,"CTriggerConfig:%s,Parse config error:%s\n", __FUNCTION__,xmlDoc.ErrorDesc());
		return -1;
	}

	TiXmlHandle docHandle(&xmlDoc);	
	TiXmlElement *pTrigger = xmlDoc.RootElement();
    if (pTrigger == NULL)
    {
        SS_XLOG(XLOG_ERROR, "CTriggerConfig::%s xml without root node\n",__FUNCTION__);
        return -1;
    }		
    
	for (TiXmlElement *pMsg = pTrigger->FirstChildElement("service"); 
         pMsg != NULL;
         pMsg = pMsg->NextSiblingElement("service"))
	{
        int nTriggerId = 0;
        if (pMsg->QueryValueAttribute("id", &nTriggerId) != TIXML_SUCCESS)
    	{
    		SS_XLOG(XLOG_WARNING,"CTriggerConfig::%s, id not found\n",__FUNCTION__);
            return -1;
    	}        
		STriggerType sType;        
		for (TiXmlElement *pItem = pMsg->FirstChildElement("item"); 
             pItem != NULL;
             pItem = pItem->NextSiblingElement("item"))
        {
            int nServId = 0, nMsgId = 0, nCsosLog = 0;
            if (pItem->QueryValueAttribute("serviceid", &nServId) != TIXML_SUCCESS ||
                pItem->QueryValueAttribute("msgid", &nMsgId) != TIXML_SUCCESS)
            {
            	SS_XLOG(XLOG_WARNING,"CTriggerConfig::%s, servid or msgid not found\n",__FUNCTION__);
                return -1;
            } 
			pItem->QueryValueAttribute("csoslog", &nCsosLog);
            TiXmlElement *pRequest  = pItem->FirstChildElement("request");            
            if (pRequest == NULL)
            {
                SS_XLOG(XLOG_WARNING,"CTriggerConfig::%s, request not found\n",__FUNCTION__);
                return -1;
            }
            STriggerItem sItem;
			sItem.nCsosLog = nCsosLog;
            if (LoadFiled(pRequest,  sItem.vecReq) != 0)
            {
                return -1;
            }            
            TiXmlElement *pResponse = pItem->FirstChildElement("response");
            if (pResponse == NULL)
            {
                SS_XLOG(XLOG_WARNING,"CTriggerConfig::%s, response not found\n",__FUNCTION__);
                return -1;
            }
                        
            if (LoadFiled(pResponse, sItem.vecRes) != 0)
            {
                return -1;
            }

            TiXmlElement *pData = pItem->FirstChildElement("data");
            if (pData != NULL)
            {
                if (LoadDataFiled(pData, sItem.vecDataDef) != 0)
                {
                    return -1;
                }
                sItem.nDataDefSize = sItem.vecDataDef.size();
            }

            sItem.nReqSize = sItem.vecReq.size();
            sItem.nResSize = sItem.vecRes.size();
            
            if ( nServId == -1 && nMsgId == -1)
            {
                sType.defaultItem = sItem;
            }
            else
            {
                sType.mapInfo.insert( make_pair(SComposeKey(nServId, nMsgId) ,sItem) );
            }
        }
        m_mapTrigger.insert( make_pair(nTriggerId, sType) );
	} 
    return 0;
}

int CTriggerConfig::LoadFiled(TiXmlElement *pItem, vecTriggerItem& vecItem)
{        
    for(TiXmlElement *pField = pItem->FirstChildElement("field");
        pField != NULL;
        pField = pField->NextSiblingElement("field"))		     
    {             
        STriggerInfo sInfo;        
        if(pField->QueryValueAttribute("name", &sInfo.strName) != TIXML_SUCCESS)
        {
            SS_XLOG(XLOG_WARNING,"CTriggerConfig::%s, get 'name' failed.\n",__FUNCTION__);
            return -1;
        } 	

        sInfo.bIsProcValue = false;
        char szIsProc[32] = {0};
        if(pField->QueryValueAttribute("isproc", &szIsProc) == TIXML_SUCCESS && strcmp(szIsProc, "true") == 0)
        {
            sInfo.bIsProcValue = true;
        }

        char szType[32] = {0};			
        if(pField->QueryValueAttribute("type", &szType) != TIXML_SUCCESS)
        {
            SS_XLOG(XLOG_WARNING,"CTriggerConfig::%s, get 'type' failed.\n",__FUNCTION__);
            return -1;
        }

        sInfo.nKey = 0;
        if (sInfo.bIsProcValue)
        {
            if(strcmp(szType,"int") == 0)
            {
                sInfo.pFunc = GetIntDefValue;
            }
            else
            {
                sInfo.pFunc = GetStringDefValue;
            }
        }
        else if(strcmp(szType,"string")==0)
        {
            sInfo.pFunc = GetStringValue;
        }
        else if(strcmp(szType,"int")==0)
        {
            sInfo.pFunc = GetIntValue;
        }
        else if(strcmp(szType,"string_array")==0)
        {
            sInfo.pFunc = GetStringArray;
        }
        else if(strcmp(szType,"int_array")==0)
        {
            sInfo.pFunc = GetIntArray;
        }
        else if(strcmp(szType,"keyvalue")==0)
        {
            sInfo.pFunc = GetKeyValue;
        }
        else if(strcmp(szType,"keyvalue_array")==0)
        {            
            if(pField->QueryValueAttribute("key", &sInfo.nKey) != TIXML_SUCCESS)
            {
                SS_XLOG(XLOG_WARNING,"CTriggerConfig::%s, 'key' not set\n",__FUNCTION__);            
            }
            sInfo.pFunc = GetKeyValueArray;            
        }
        else if(strcmp(szType,"password")==0)
        {
            sInfo.pFunc = GetPassword; 
        }
        else
        {
            sInfo.pFunc = NULL;
        }

        if(pField->QueryValueAttribute("default", &sInfo.strDefault) != TIXML_SUCCESS)
        {
            SS_XLOG(XLOG_TRACE,"CTriggerConfig::%s, 'default' not set\n",__FUNCTION__);            
        }
        sInfo.keyType = 0;		
        if(pField->QueryValueAttribute("keytype", &sInfo.keyType) != TIXML_SUCCESS)
        {
            ;//SS_XLOG(XLOG_DEBUG,"CTriggerConfig::%s, 'keytype' not set\n",__FUNCTION__);            
        }
        if (sInfo.keyType <0)
        {
            sInfo.keyType = 3;
            SS_XLOG(XLOG_WARNING, "CTriggerConfig::%s, 'keytype'[%d] out range, set keyType=3\n",__FUNCTION__,sInfo.keyType);
        }

        vecItem.push_back(sInfo);
    }  
    return 0;
}

int CTriggerConfig::LoadDataFiled(TiXmlElement *pItem, vecTriggerItem& vecItem)
{        
    for(TiXmlElement *pField = pItem->FirstChildElement("field");
        pField != NULL;
        pField = pField->NextSiblingElement("field"))		     
    {             
        STriggerInfo sInfo;        
        if(pField->QueryValueAttribute("name", &sInfo.strName) != TIXML_SUCCESS)
        {
            SS_XLOG(XLOG_WARNING,"CTriggerConfig::%s, get 'name' failed.\n",__FUNCTION__);
            return -1;
        } 	

        sInfo.bIsProcValue = true;
        char szType[32] = {0};
        if(pField->QueryValueAttribute("type", &szType) != TIXML_SUCCESS)
        {
            SS_XLOG(XLOG_WARNING,"CTriggerConfig::%s, get 'type' failed.\n",__FUNCTION__);
            return -1;
        }

        if(strcmp(szType,"int") == 0)
        {
            sInfo.pFunc = GetIntDefValue;
        }
        else
        {
            sInfo.pFunc = GetStringDefValue;
        }

        pField->QueryValueAttribute("default", &sInfo.strDefault);

        sInfo.keyType = 0;		
        pField->QueryValueAttribute("keytype", &sInfo.keyType);

        if (sInfo.keyType > 3 || sInfo.keyType <0)
        {
            sInfo.keyType = 3;
            SS_XLOG(XLOG_WARNING, "CTriggerConfig::%s, 'keytype'[%d] out range, set keyType=3\n",__FUNCTION__,sInfo.keyType);
        }

        vecItem.push_back(sInfo);
    }  
    return 0;
}


void CTriggerConfig::Dump()
{
    SS_XLOG(XLOG_DEBUG,"-------CTriggerConfig::Dump begin---------\n");    
    mapTrigger::iterator itrMap;
    for(itrMap=m_mapTrigger.begin(); itrMap!=m_mapTrigger.end(); ++itrMap)
    {
        SS_XLOG(XLOG_DEBUG,"  id[%d]\n", itrMap->first);
        STriggerType& sType = itrMap->second;
        
        SS_XLOG(XLOG_DEBUG," common: req[%d], res[%d]\n", sType.defaultItem.nReqSize, sType.defaultItem.nResSize);
        SS_XLOG(XLOG_DEBUG," request\n");
        vecTriggerItem::iterator itr;
        for(itr = sType.defaultItem.vecReq.begin();itr != sType.defaultItem.vecReq.end(); ++itr)
        {
            SS_XLOG(XLOG_DEBUG,"   name[%s]\n", itr->strName.c_str());
        }
        SS_XLOG(XLOG_DEBUG," response\n");
        for(itr = sType.defaultItem.vecRes.begin();itr != sType.defaultItem.vecRes.end(); ++itr)
        {
            SS_XLOG(XLOG_DEBUG,"   name[%s]\n", itr->strName.c_str());
        }
        SS_XLOG(XLOG_DEBUG,"  specified items:\n");
        mapTriggerInfo::iterator itrInfo;
        for(itrInfo=sType.mapInfo.begin(); itrInfo!=sType.mapInfo.end(); ++itrInfo)
        {
            const SComposeKey &sKey = itrInfo->first;
            STriggerItem &sItem = itrInfo->second;
            SS_XLOG(XLOG_DEBUG,"   servid[%d],msgid[%d]\n", sKey.dwServiceId, sKey.dwMsgId);
            SS_XLOG(XLOG_DEBUG,"   common: req[%d], res[%d]\n", sItem.nReqSize, sItem.nResSize);
            SS_XLOG(XLOG_DEBUG,"   request\n");            
            for(itr = sItem.vecReq.begin();itr != sItem.vecReq.end(); ++itr)
            {
                SS_XLOG(XLOG_DEBUG,"   name[%s]\n", itr->strName.c_str());
            }
            SS_XLOG(XLOG_DEBUG,"   response\n");
            for(itr = sItem.vecRes.begin();itr != sItem.vecRes.end(); ++itr)
            {
                SS_XLOG(XLOG_DEBUG,"   name[%s]\n", itr->strName.c_str());
            }            
        }
    }
    SS_XLOG(XLOG_DEBUG,"-------CTriggerConfig::Dump end--------\n");
}

string GetIntValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved)
{	
    int dwValue;
    if (0 == pHandler->GetValue(strKey.c_str(), &dwValue))
	{
		char szTemp[16];
        snprintf(szTemp, sizeof(szTemp), "%d", dwValue);
		return string(szTemp);
	}

    return sPara.strDefault;
}

string GetKeyValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved)
{
    void * pValue = NULL;
    int nLen = 0;
    if (0 == pHandler->GetValue(strKey.c_str(), &pValue, &nLen) &&
        nLen > 4)
    {           
        return string((const char*)pValue+4, nLen-4);
    }
    return sPara.strDefault;
}

string GetStringValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved)
{   
    void * pValue = NULL;
    int nLen = 0;
    if (0 == pHandler->GetValue(strKey.c_str(), &pValue, &nLen))
    {          
        int nRealLen = Remove0A0D((char*)pValue, nLen);
        return string((const char*)pValue, nRealLen);
    }
    return sPara.strDefault;
}

string GetStringArray(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved)
{           
    vector<SStructValue> vecData;
    pHandler->GetValues(strKey.c_str(), vecData); 
    if (vecData.empty())
    {        
        return sPara.strDefault;        
    }
    else
    {
        string strValue("|");
        for(vector<SStructValue>::iterator itr = vecData.begin();
            itr != vecData.end();
            ++itr)
        {
            const SStructValue& sValue = *itr;
            strValue.append(string((const char*)sValue.pValue, sValue.nLen));
            strValue.append("|");
        }
        //strValue.erase(strValue.end()-1);       
        return strValue;
    }    
}

string GetIntArray(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved)
{          
    vector<SStructValue> vecData;
    pHandler->GetValues(strKey.c_str(), vecData); 
    if (vecData.empty())
    {        
        return sPara.strDefault;
    }
    else
    {
        string strValue("|");
        char szTemp[16] = {0};
        for(vector<SStructValue>::iterator itr = vecData.begin();
            itr != vecData.end();
            ++itr)
        {
            const SStructValue& sValue = *itr;            
            snprintf(szTemp, sizeof(szTemp)-1, "%d|", *static_cast<int*>(sValue.pValue));
            strValue.append(szTemp);        
        }
        return strValue;
    }    
}

string GetKeyValueArray(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved)
{
    vector<SStructValue> vecData;
    pHandler->GetValues(strKey.c_str(),vecData);                
    for(vector<SStructValue>::iterator itr=vecData.begin(); itr!=vecData.end(); ++itr)
    {
        const SStructValue &keyvalue = *itr;     
        if((int)ntohl(*(int *)keyvalue.pValue) == sPara.nKey)
        {
            return string((char *)keyvalue.pValue+4, keyvalue.nLen-4);                     
        }
    } 
    return sPara.strDefault;
}

string GetPassword(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pReserved)
{   
    void * pValue = NULL;
    int nLen = 0;
    if (0 == pHandler->GetValue(strKey.c_str(), &pValue, &nLen))
    {                
        char *pSep1 = (char*)memchr(pValue, ':', nLen);
        if (pSep1 == NULL)
        {
            return "******";//ekey/voice
        }
        string str; 
        char *pEnd = (char*)pValue + nLen;
        char *pos = (char*)pValue;        
        char *pSep2 = pos;
        while (pSep1 != NULL && pSep2 != NULL && pSep1-pSep2>0)
        {            
            str.append(pSep2, pSep1-pSep2);
            pos = pSep1;
            pSep1 = (char*)memchr(pos+1, ':', pEnd-pos-1);
            pSep2 = (char*)memchr(pos+1, '|', pEnd-pos-1);
        }    
        return str;
    }
    return sPara.strDefault;
}

string GetIntDefValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pDefValues)
{
    string strOut;
    if (!pDefValues)
    {
        return strOut;
    }
    map<string, vector<string> >* pMapDefValue = (map<string, vector<string> >*)pDefValues;
    map<string, vector<string> >::const_iterator itrDefValue = pMapDefValue->find(strKey);
    if (itrDefValue == pMapDefValue->end() || itrDefValue->second.empty())
    {
        return strOut;
    }

    vector<string>::const_iterator itrValue = itrDefValue->second.begin();
    for (; itrValue != itrDefValue->second.end(); ++itrValue)
    {
        char szTemp[16];
        snprintf(szTemp, sizeof(szTemp)-1, "%d", *(const int*)itrValue->c_str());
        if (!strOut.empty())
        {
            strOut += "|";
        }
        strOut += szTemp;
    }

    return strOut;
}

string GetStringDefValue(const string& strKey, CAvenueMsgHandler* pHandler, const SLogPara& sPara, const void* pDefValues)
{
    string strOut;
    if (!pDefValues)
    {
        return strOut;
    }
    map<string, vector<string> >* pMapDefValue = (map<string, vector<string> >*)pDefValues;
    map<string, vector<string> >::const_iterator itrDefValue = pMapDefValue->find(strKey);
    if (itrDefValue == pMapDefValue->end() || itrDefValue->second.empty())
    {
        return strOut;
    }

    vector<string>::const_iterator itrValue = itrDefValue->second.begin();
    for (; itrValue != itrDefValue->second.end(); ++itrValue)
    {
        if (!strOut.empty())
        {
            strOut += "|";
        }
        strOut.append(*itrValue);
    }

    return strOut;
}

int Remove0A0D(char* pAccount, int nLen)
{
	return nLen;
	int nNum = 0;
    if (pAccount[nLen-1] == 0x0A || pAccount[nLen-1] == 0x0D)
    {
        nNum++;
        pAccount[nLen-1] = '\0';
    }
    
    for(int i=0; i<nLen-nNum;)
	{
		if (pAccount[i] == 0x0A || pAccount[i] == 0x0D)
		{            
            for(int j=i; j<nLen-1;++j)
            {
                pAccount[j] = pAccount[j+1];
            }	
            nNum++;         
		}
        else
        {
            i++;
        }
	}
//	    if (nNum > 0)
//	    {
//	        memset(pAccount+(nLen-nNum), 0, nNum);
//	    }
    return nLen-nNum;
}
