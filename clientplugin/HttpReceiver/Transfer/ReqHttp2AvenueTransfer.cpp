#include "ReqHttp2AvenueTransfer.h"
#include "AvenueHttpTransferLogHelper.h"
#include "DirReader.h"
#include "HpsCommon.h"
#include <boost/algorithm/string.hpp>



#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 128
#endif

map<string,SSoUnit> CReqHttp2AvenueTransfer::sm_mapSo;
boost::mutex CReqHttp2AvenueTransfer::sm_mut;
string CReqHttp2AvenueTransfer::sm_strReloadErrorMsg = "";
int CReqHttp2AvenueTransfer::sm_nReloadResult = 0;
bool CReqHttp2AvenueTransfer::sm_bReloadFlag = true;
boost::condition_variable  CReqHttp2AvenueTransfer::sm_cond;


CReqHttp2AvenueTransfer::CReqHttp2AvenueTransfer()
{
}

CReqHttp2AvenueTransfer::~CReqHttp2AvenueTransfer()
{
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s\n",__FUNCTION__);

	map<string,STransferUnit>::iterator iterMap;
	for(iterMap = m_mapUnitBySoName.begin(); iterMap != m_mapUnitBySoName.end(); ++ iterMap)
	{	
		STransferUnit sUnit = iterMap->second;	
		destroy_obj pFunDestroy = sUnit.pFunDestroy;
		pFunDestroy(sUnit.pHttpTransferObj);	
	}
	
	m_mapUnitBySoName.clear();	
	m_mapTransferUnit.clear();	

	boost::unique_lock<boost::mutex> lock(sm_mut);
	map<string,SSoUnit>::iterator iterSmMap;
	for(iterSmMap = sm_mapSo.begin(); iterSmMap != sm_mapSo.end();++iterSmMap)
	{			
		if(iterSmMap->second.nLoadNum == 0)
		{
            CLOSE_LIBRARY(iterSmMap->second.pHandle);
			sm_mapSo.erase(iterSmMap);
			m_mapSoInfoBySoName.erase(iterSmMap->first);
		}
		else
		{
			iterSmMap->second.nLoadNum--;
		}
	}
	sm_strReloadErrorMsg = "";
}

int CReqHttp2AvenueTransfer::DoLoadTransferObj(const string & strDirPath, const string & strFilter)
{	
	boost::unique_lock<boost::mutex> lock(sm_mut);
		
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s, strDirPath[%s], strFilter[%s]\n",__FUNCTION__,strDirPath.c_str(),strFilter.c_str());

	nsHpsTransfer::CDirReader oDirReader(strFilter.c_str());
	if(!oDirReader.OpenDir(strDirPath.c_str()))
	{
		TS_XLOG(XLOG_ERROR,"CReqHttp2AvenueTransfer::%s,open dir error\n",__FUNCTION__);
		return -1;
	}

	char szFileName[MAX_PATH] = {0};

	if(!oDirReader.GetFirstFilePath(szFileName))
	{
		TS_XLOG(XLOG_ERROR,"CReqHttp2AvenueTransfer::%s,GetFirstFilePath error\n",__FUNCTION__);
		return 0;
	}
	
	do
	{
		if(0 != InstallSoPlugin(szFileName))
		{
			TS_XLOG(XLOG_ERROR,"CReqHttp2AvenueTransfer::%s,InstallSoPlugin[%s] error\n",__FUNCTION__,szFileName);
		}		
		
	}while(oDirReader.GetNextFilePath(szFileName));
	
	CreateTransferObjs();

	return 0;
}

int CReqHttp2AvenueTransfer::InstallSoPlugin(const char *szSoName)
{
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s, szSoName[%s]\n",__FUNCTION__,szSoName);

	map<string,SSoUnit>::iterator itr = sm_mapSo.find(szSoName);
	if(itr != sm_mapSo.end())
	{
		return 0;
	}
	
    PluginPtr pHandle = LOAD_LIBRARY(szSoName);
    if (NULL == pHandle)
	{
        TS_XLOG(XLOG_ERROR, "CReqHttp2AvenueTransfer::%s, LOAD_LIBRARY[%s] error[%s]\n", __FUNCTION__, szSoName, GET_ERROR_INFO("can't Load"));
        return -1;
	}
	
	TS_XLOG(XLOG_TRACE,"CReqHttp2AvenueTransfer::%s-------------pHandle[%0X]--------------  \n",__FUNCTION__,pHandle);

    create_obj create_unit = (create_obj)GET_FUNC_PTR(pHandle, "create");
    if (!create_unit)
    {
        TS_XLOG(XLOG_ERROR, "CReqHttp2AvenueTransfer::%s, GET_FUNC_PTR create[%s] error[%s]\n", __FUNCTION__, szSoName, GET_ERROR_INFO("can't find"));
        return -1;                    
    }

    destroy_obj destroy_unit = (destroy_obj)GET_FUNC_PTR(pHandle, "destroy");
    if (!destroy_unit)
    {
        TS_XLOG(XLOG_ERROR, "CReqHttp2AvenueTransfer::%s, GET_FUNC_PTR destroy[%s] error[%s]\n", __FUNCTION__, szSoName, GET_ERROR_INFO("can't find"));
        return -1;
    }
	
	SSoUnit sSoUnit;
	sSoUnit.strSoName = szSoName;
	sSoUnit.pHandle = pHandle;
	sSoUnit.pFunCreate = create_unit;
	sSoUnit.pFunDestroy = destroy_unit;

	sm_mapSo.insert(make_pair(szSoName,sSoUnit));

	return 0;
}


int CReqHttp2AvenueTransfer::CreateTransferObjs()
{
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s\n",__FUNCTION__);

	map<string,SSoUnit>::iterator iter;
	for(iter = sm_mapSo.begin(); iter != sm_mapSo.end(); ++iter)
	{
		string strSoName = iter->first;
		/* if m_mapUnitBySoName has the SoName, don't create class instance */
		map<string,STransferUnit>::iterator iterMap = m_mapUnitBySoName.find(strSoName);
		if(iterMap == m_mapUnitBySoName.end())
		{				
			create_obj create_unit = iter->second.pFunCreate;
			destroy_obj destroy_unit = iter->second.pFunDestroy;
			
			IAvenueHttpTransfer* pTransferObj = create_unit();
	        if(pTransferObj == NULL)
	        {
	            TS_XLOG(XLOG_ERROR,"CReqHttp2AvenueTransfer::%s, create object failed, SoName[%s]\n",__FUNCTION__,strSoName.c_str());
	            continue;
	        }
			
			string strTemp = pTransferObj->GetPluginSoInfo();
			TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s, GetPluginSoInfo(), [%s]\n",__FUNCTION__, strTemp.c_str());
			m_mapSoInfoBySoName.insert(make_pair(strSoName,strTemp));

			STransferUnit oSTUnit(strSoName,pTransferObj, destroy_unit);

			vector<string> vecTemp;
			pTransferObj->GetUriMapping(vecTemp);
			vector<string>::iterator iterTemp;
			for(iterTemp=vecTemp.begin();iterTemp!=vecTemp.end();++iterTemp)
			{
				vector<string> vecSplit;
				boost::algorithm::split( vecSplit, (*iterTemp), boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);  
				if( vecSplit.size() == 3 )
				{
					string strUrl = vecSplit[0];
					transform(strUrl.begin(), strUrl.end(), strUrl.begin(), ::tolower);
					oSTUnit.vecUrl.push_back(strUrl);
					oSTUnit.dwServiceId = atoi(vecSplit[1].c_str());
					oSTUnit.dwMessageId = atoi(vecSplit[2].c_str());
					m_mapTransferUnit.insert(make_pair(strUrl,oSTUnit));
				}
			}
			m_mapUnitBySoName.insert(make_pair(strSoName,oSTUnit));
			
			iter->second.nLoadNum++;
		}
		else
		{
			continue;
		}
	}

	return 0;
}

int CReqHttp2AvenueTransfer::UnLoadTransferObj(const string &strDirPath,const vector<string> &vecSo)
{
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s, vecSo.size[%d]\n",__FUNCTION__,vecSo.size());

	boost::unique_lock<boost::mutex> lock(sm_mut);
	sm_strReloadErrorMsg = "";
	int nCount = 0;

	vector<string>::const_iterator iterVec;
	for(iterVec = vecSo.begin(); iterVec != vecSo.end(); ++iterVec)
	{
		string strSoName = *iterVec;
		string strFullPath = strDirPath + CST_PATH_SEP + strSoName;
		map<string,STransferUnit>::iterator iterMap = m_mapUnitBySoName.find(strFullPath);
		if(iterMap != m_mapUnitBySoName.end())
		{	
			/*destroy the instance of so*/
			STransferUnit sUnit = iterMap->second;
			TS_XLOG(XLOG_TRACE,"CReqHttp2AvenueTransfer::%s, m_mapUnitBySoName.erase so_map[%s]\n",__FUNCTION__,sUnit.strSoName.c_str());	
			destroy_obj pFunDestroy = sUnit.pFunDestroy;
			pFunDestroy(sUnit.pHttpTransferObj);
			m_mapUnitBySoName.erase(iterMap);

			/*erase the m_mapTransferUnit*/	
			vector<string>::iterator itrSrv;
			for(itrSrv = sUnit.vecUrl.begin(); itrSrv != sUnit.vecUrl.end(); ++itrSrv)
			{
				m_mapTransferUnit.erase(*itrSrv);
			}
		}
		else
		{			
			TS_XLOG(XLOG_WARNING,"CReqHttp2AvenueTransfer::%s, There is no such so in m_mapUnitBySoName[%s]\n",__FUNCTION__,strFullPath.c_str());
		}	

		
		
		sm_nReloadResult = 0;
		map<string,SSoUnit>::iterator iterSmMap = sm_mapSo.find(strFullPath);
		if(iterSmMap != sm_mapSo.end())
		{		
			iterSmMap->second.nLoadNum--;
			TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s, soName[%s],sm_nLoadNum[%d]\n",__FUNCTION__,strFullPath.c_str(),iterSmMap->second.nLoadNum);
			if(iterSmMap->second.nLoadNum == 0)
			{
				TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s,dlcolse so[%s]\n",__FUNCTION__,strFullPath.c_str());
				/*execute in the last thread*/
                CLOSE_LIBRARY(iterSmMap->second.pHandle);
				m_mapSoInfoBySoName.erase(iterSmMap->first);
				sm_mapSo.erase(iterSmMap);
				sm_strReloadErrorMsg += string("Unload so[") + strSoName + "] success;";

				nCount++;
			}
		}
		else
		{
			TS_XLOG(XLOG_WARNING,"CReqHttp2AvenueTransfer::%s, There is no such so in sm_mapSo[%s]\n",__FUNCTION__,strSoName.c_str());
			sm_nReloadResult = -2;
			sm_strReloadErrorMsg += string("so[") + strSoName + "] hasn't been loaded;";
			nCount++;
		}
	}

	/* true if in the last thread, notify the condition singal*/
	if(nCount == (int)vecSo.size())
	{
		TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s, In the last thread, notify\n",__FUNCTION__);
		sm_cond.notify_one();
	}
	return 0;
}

int CReqHttp2AvenueTransfer::ReLoadTransferObj(const string & strDirPath, const vector<string> &vecSoName)
{	
	TS_XLOG(XLOG_TRACE,"CReqHttp2AvenueTransfer::%s\n",__FUNCTION__);
	
	boost::unique_lock<boost::mutex> lock(sm_mut);	
	
	int nRet = -1;
	if(sm_bReloadFlag)
	{
		sm_strReloadErrorMsg = "";
		sm_nReloadResult = -2;
		vector<string>::const_iterator iterVec;
		for(iterVec = vecSoName.begin(); iterVec != vecSoName.end(); ++iterVec)
		{		
			char szTransferObj[MAX_PATH] = {0};
			sprintf(szTransferObj, "%s%s%s", strDirPath.c_str(), CST_PATH_SEP, (*iterVec).c_str());
			TS_XLOG(XLOG_TRACE,"CReqHttp2AvenueTransfer::%s, szTransferObj[%s]\n",__FUNCTION__,szTransferObj);
			map<string,SSoUnit>::iterator iterSmMap = sm_mapSo.find(szTransferObj);
			if(iterSmMap == sm_mapSo.end())
			{
				if(0 != InstallSoPlugin(szTransferObj))
				{
					TS_XLOG(XLOG_WARNING,"CReqHttp2AvenueTransfer::%s, instatll szTransferObj[%s] failed\n",__FUNCTION__,szTransferObj);
					sm_strReloadErrorMsg += string("Reload ") + szTransferObj + " failed;";
					sm_nReloadResult = -1;
					nRet = -1;
				}
				else
				{
					sm_nReloadResult = 0;
					nRet = 0;

					sm_strReloadErrorMsg += string("Reload ") + szTransferObj + " OK;";
				}
			}
			else
			{
				TS_XLOG(XLOG_WARNING,"CReqHttp2AvenueTransfer::%s, szTransferObj[%s] haven't been unloaded\n",__FUNCTION__,szTransferObj);
				sm_strReloadErrorMsg += string(szTransferObj) + " haven't been unloaded;";
			}
		}	
		sm_bReloadFlag = false;
	}

	CreateTransferObjs();

	sm_cond.notify_one();

	return nRet;
}

int CReqHttp2AvenueTransfer::ReLoadCommonTransfer(const string &strTransferName)
{
	TS_XLOG(XLOG_TRACE,"CReqHttp2AvenueTransfer::%s\n",__FUNCTION__);
	boost::unique_lock<boost::mutex> lock(sm_mut);
	sm_strReloadErrorMsg = "";
	sm_nReloadResult = 0;

	map<string,STransferUnit>::iterator iterMap = m_mapUnitBySoName.find(strTransferName);
	if(iterMap != m_mapUnitBySoName.end())
	{	
		/*destroy the instance of so*/
		STransferUnit& sUnit = iterMap->second;
		TS_XLOG(XLOG_TRACE,"CReqHttp2AvenueTransfer::%s, m_mapUnitBySoName.erase so_map[%s]\n",__FUNCTION__,sUnit.strSoName.c_str());	
		destroy_obj pFunDestroy = sUnit.pFunDestroy;
		pFunDestroy(sUnit.pHttpTransferObj);
		vector<string>::iterator itrSrv;
		for(itrSrv = sUnit.vecUrl.begin(); itrSrv != sUnit.vecUrl.end(); ++itrSrv)
		{
			m_mapTransferUnit.erase(*itrSrv);
		}
		m_mapUnitBySoName.erase(iterMap);
	}

	map<string,SSoUnit>::iterator iterSoUnit = sm_mapSo.find(strTransferName);
	if (iterSoUnit != sm_mapSo.end())
	{
		create_obj create_unit = iterSoUnit->second.pFunCreate;
		destroy_obj destroy_unit = iterSoUnit->second.pFunDestroy;

		IAvenueHttpTransfer* pTransferObj = create_unit();
		if(pTransferObj == NULL)
		{
			TS_XLOG(XLOG_ERROR,"CReqHttp2AvenueTransfer::%s, create object failed, SoName[%s]\n",__FUNCTION__,strTransferName.c_str());
			sm_nReloadResult = -21;
			sm_cond.notify_one();
			return -21;
		}

		STransferUnit oSTUnit(strTransferName,pTransferObj, destroy_unit);

		vector<string> vecTemp;
		pTransferObj->GetUriMapping(vecTemp);
		vector<string>::iterator iterTemp;
		for(iterTemp=vecTemp.begin();iterTemp!=vecTemp.end();++iterTemp)
		{
			vector<string> vecSplit;
			boost::algorithm::split( vecSplit, (*iterTemp), boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);  
			if( vecSplit.size() == 3 )
			{
				string strUrl = vecSplit[0];
				transform(strUrl.begin(), strUrl.end(), strUrl.begin(), ::tolower);
				oSTUnit.vecUrl.push_back(strUrl);

				m_mapTransferUnit.insert(make_pair(strUrl,oSTUnit));
			}
		}
		m_mapUnitBySoName.insert(make_pair(strTransferName,oSTUnit));
		sm_cond.notify_one();
		sm_nReloadResult = 0;
		return 0;
	}
	sm_nReloadResult = -22;
	sm_cond.notify_one();
	return -22;
}

string CReqHttp2AvenueTransfer::GetPluginSoInfo()
{
	TS_XLOG(XLOG_TRACE,"CReqHttp2AvenueTransfer::%s\n",__FUNCTION__);

	string strPluginSoInfo = "";
	map<string,string>::iterator iter;
	for(iter = m_mapSoInfoBySoName.begin(); iter != m_mapSoInfoBySoName.end();++iter)
	{
		strPluginSoInfo += iter->second;
	}
	return strPluginSoInfo;
}

int CReqHttp2AvenueTransfer::WaitForReload(string &strReloadErorMsg)
{
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s\n",__FUNCTION__);
	boost::unique_lock<boost::mutex> lock(sm_mut);   	
	sm_cond.timed_wait<boost::posix_time::seconds>(lock, boost::posix_time::seconds(5));
	
	strReloadErorMsg = sm_strReloadErrorMsg;

	TS_XLOG(XLOG_TRACE,"CReqHttp2AvenueTransfer::%s, sm_nReloadResult[%d], strReloadErorMsg[%s]\n",
		__FUNCTION__,sm_nReloadResult,sm_strReloadErrorMsg.c_str());
	return sm_nReloadResult;
}


void CReqHttp2AvenueTransfer::GetUriMapping(vector<string>& vecUriMapping)
{	
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s\n",__FUNCTION__);
	map<string,STransferUnit>::iterator iter;
	for(iter = m_mapUnitBySoName.begin(); iter != m_mapUnitBySoName.end(); ++iter)
	{
		TS_XLOG(XLOG_TRACE,"CReqHttp2AvenueTransfer::%s, pHttpTransferObj[%0X]\n",__FUNCTION__,iter->second.pHttpTransferObj);
		
		IAvenueHttpTransfer *pTransferObj = iter->second.pHttpTransferObj;
		vector<string> vecTemp;
		pTransferObj->GetUriMapping(vecTemp);
		vector<string>::iterator iterTemp;
		for(iterTemp=vecTemp.begin();iterTemp!=vecTemp.end();++iterTemp)
		{
			vecUriMapping.push_back(*iterTemp);
		}
	}
}

SUrlInfo CReqHttp2AvenueTransfer::GetSoNameByUrl(const string& strUrl)
{
	map<string, STransferUnit>::iterator iter = m_mapTransferUnit.find(strUrl);
	if (iter == m_mapTransferUnit.end())
	{
		return SUrlInfo();
	}
	SUrlInfo info;
	info.strSoName = iter->second.strSoName;
	info.dwServiceId = iter->second.dwServiceId;
	info.dwMessageId = iter->second.dwMessageId;
	return info;
}

int CReqHttp2AvenueTransfer::DoTransferRequestMsg(const string& strUrlIdentify, IN unsigned int dwServiceId,IN unsigned int dwMsgId, IN unsigned int dwSequence, IN const char *szUriCommand, 
	IN const char *szUriAttribute, IN const char* szRemoteIp, IN const unsigned int dwRemotePort, OUT void *ppAvenuePacket , OUT int *pOutLen)
{
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s, serviceId[%u]\n",__FUNCTION__,dwServiceId);
	string strUrlId = strUrlIdentify;
	transform(strUrlId.begin(), strUrlId.end(), strUrlId.begin(), (int(*)(int))tolower);
	map<string, STransferUnit>::iterator itrTUnit = m_mapTransferUnit.find(strUrlId);
    if(itrTUnit != m_mapTransferUnit.end())
    {
    	STransferUnit& rTUnit = itrTUnit->second;		
		IAvenueHttpTransfer *pTransferObj = rTUnit.pHttpTransferObj;
		int ret = pTransferObj->RequestTransfer(dwServiceId, dwMsgId, dwSequence, szUriCommand, szUriAttribute, szRemoteIp,dwRemotePort, ppAvenuePacket,pOutLen);
		TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s, shared Transfer method ret[%d]\n",__FUNCTION__,ret);
        if(0 != ret)
    	{
    		/* return the error code */
			TS_XLOG(XLOG_ERROR,"CReqHttp2AvenueTransfer::%s,transfer request msg error, serviceid[%u], ret[%d]\n",
					__FUNCTION__,dwServiceId,ret);
			return (ret == ERROR_BAD_REQUEST_INNER) ? ERROR_BAD_REQUEST_INNER : ERROR_TRANSFER_REQUEST_FAILED;;
        }
		else
		{
			return 0;
		}
    }
    else
    {
        TS_XLOG(XLOG_WARNING,"CReqHttp2AvenueTransfer::%s,can't find libso, szUriCommand[%s]\n",__FUNCTION__,szUriCommand);
        return ERROR_TRANSFER_NOT_FOUND;
    } 
}

int CReqHttp2AvenueTransfer::DoTransferResponseMsg(const string& strUrlIdentify, IN unsigned int dwServiceId,IN unsigned int dwMsgId,IN const void *pAvenuePacket,
		IN unsigned int nLen,OUT void *ppHttpPacket,OUT int * pOutLen)
{
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s, serviceId[%u]\n",__FUNCTION__,dwServiceId);
	string strUrlId = strUrlIdentify;
	transform(strUrlId.begin(), strUrlId.end(), strUrlId.begin(), (int(*)(int))tolower);	
	map<string, STransferUnit>::iterator itrTUnit = m_mapTransferUnit.find(strUrlId);
    if(itrTUnit != m_mapTransferUnit.end())
    {
		IAvenueHttpTransfer *pTransferObj = itrTUnit->second.pHttpTransferObj;
		int nRet = pTransferObj->ResponseTransfer(pAvenuePacket, nLen, ppHttpPacket,pOutLen);
		return nRet >=0 ? nRet : ERROR_TRANSFER_RESPONSE_FAILED;
    }
    else
    {
        TS_XLOG(XLOG_WARNING,"CReqHttp2AvenueTransfer::%s,can't find libso, serviceid[%u]\n",__FUNCTION__,dwServiceId);
        return ERROR_TRANSFER_NOT_FOUND;
    } 
}

void CReqHttp2AvenueTransfer::CleanRubbish()
{
	TS_XLOG(XLOG_DEBUG,"CReqHttp2AvenueTransfer::%s\n",__FUNCTION__);
	map<string,STransferUnit>::iterator iter;
	for(iter = m_mapUnitBySoName.begin(); iter != m_mapUnitBySoName.end(); ++iter)
	{		
		STransferUnit& rTUnit = iter->second;
		IAvenueHttpTransfer *pTransferObj = rTUnit.pHttpTransferObj;
		pTransferObj->CleanRubbish();
	}
}

void CReqHttp2AvenueTransfer::Dump()
{
	TS_XLOG(XLOG_NOTICE,"============CReqHttp2AvenueTransfer::%s ==============\n",__FUNCTION__);
	map<string, STransferUnit>::iterator iter;
	for(iter = m_mapTransferUnit.begin(); iter != m_mapTransferUnit.end(); ++iter)
	{
		TS_XLOG(XLOG_NOTICE,"          <%s, %s, %0X>\n",iter->first.c_str(), iter->second.strSoName.c_str(),iter->second.pHttpTransferObj);
	}

	TS_XLOG(XLOG_NOTICE,"============CReqHttp2AvenueTransfer::%s  end==============\n",__FUNCTION__);
}


