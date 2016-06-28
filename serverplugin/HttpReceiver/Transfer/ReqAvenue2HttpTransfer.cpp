#include "ReqAvenue2HttpTransfer.h"
#include "AvenueHttpTransferLogHelper.h"
#include "DirReader.h"
#include "HpsCommon.h"
#include <boost/algorithm/string.hpp>


#ifndef MAX_PATH
#define MAX_PATH 256
#endif

map<string,SA2HSoUnit> CReqAvenue2HttpTransfer::sm_mapSo;
boost::mutex CReqAvenue2HttpTransfer::sm_mut;
string CReqAvenue2HttpTransfer::sm_strReloadErrorMsg = "";
int CReqAvenue2HttpTransfer::sm_nReloadResult = 0;
bool CReqAvenue2HttpTransfer::sm_bReloadFlag = true;
boost::condition_variable  CReqAvenue2HttpTransfer::sm_cond;


CReqAvenue2HttpTransfer::CReqAvenue2HttpTransfer()
{
}

CReqAvenue2HttpTransfer::~CReqAvenue2HttpTransfer()
{
	TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s\n",__FUNCTION__);

	map<string,SA2HTransferUnit>::iterator iterMap;
	for(iterMap = m_mapUnitBySoName.begin(); iterMap != m_mapUnitBySoName.end(); ++ iterMap)
	{	
		SA2HTransferUnit sUnit = iterMap->second;	
		destroy_a2h_obj pFunDestroy = sUnit.pFunDestroy;
		pFunDestroy(sUnit.pTransferObj);	
	}
	
	m_mapUnitBySoName.clear();	
	m_mapTransferUnit.clear();	

	boost::unique_lock<boost::mutex> lock(sm_mut);
	map<string,SA2HSoUnit>::iterator iterSmMap;
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

int CReqAvenue2HttpTransfer::DoLoadTransferObj(const string & strDirPath, const string & strFilter)
{	
	boost::unique_lock<boost::mutex> lock(sm_mut);
		
	TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, strDirPath[%s], strFilter[%s]\n",__FUNCTION__,strDirPath.c_str(),strFilter.c_str());

	nsHpsTransfer::CDirReader oDirReader(strFilter.c_str());
	if(!oDirReader.OpenDir(strDirPath.c_str()))
	{
		TS_XLOG(XLOG_ERROR,"CReqAvenue2HttpTransfer::%s,open dir error\n",__FUNCTION__);
		return -1;
	}

	char szFileName[MAX_PATH] = {0};

	if(!oDirReader.GetFirstFilePath(szFileName))
	{
		TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s,GetFirstFilePath error\n",__FUNCTION__);
		return 0;
	}
	
	do
	{
		if(0 != InstallSoPlugin(szFileName))
		{
			TS_XLOG(XLOG_ERROR,"CReqAvenue2HttpTransfer::%s,InstallSoPlugin[%s] error\n",__FUNCTION__,szFileName);
		}		
		
	}while(oDirReader.GetNextFilePath(szFileName));
	
	CreateTransferObjs();

	return 0;
}

int CReqAvenue2HttpTransfer::InstallSoPlugin(const char *szSoName)
{
	TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, szSoName[%s]\n",__FUNCTION__,szSoName);

	map<string,SA2HSoUnit>::iterator itr = sm_mapSo.find(szSoName);
	if(itr != sm_mapSo.end())
	{
		return 0;
	}
	
    PluginPtr pHandle = LOAD_LIBRARY(szSoName);
    if (NULL == pHandle)
	{
        TS_XLOG(XLOG_ERROR, "CReqAvenue2HttpTransfer::%s, LOAD_LIBRARY[%s] error[%s]\n", __FUNCTION__, szSoName, GET_ERROR_INFO("can't Load"));
        return -1;
	}
	
	TS_XLOG(XLOG_TRACE,"CReqAvenue2HttpTransfer::%s-------------pHandle[%0X]--------------  \n", __FUNCTION__, pHandle);

    create_a2h_obj create_unit = (create_a2h_obj)GET_FUNC_PTR(pHandle, "create");
    if (!create_unit)
    {
        TS_XLOG(XLOG_ERROR, "CReqAvenue2HttpTransfer::%s, GET_FUNC_PTR create[%s] error[%s]\n", __FUNCTION__, szSoName, GET_ERROR_INFO("can't find"));
        return -1;                    
    }

    destroy_a2h_obj destroy_unit = (destroy_a2h_obj)GET_FUNC_PTR(pHandle, "destroy");
    if (!destroy_unit)
    {
        TS_XLOG(XLOG_ERROR, "CReqAvenue2HttpTransfer::%s, GET_FUNC_PTR destroy[%s] error[%s]\n", __FUNCTION__, szSoName, GET_ERROR_INFO("can't find"));
        return -1;
    }
	
	SA2HSoUnit SA2HSoUnit;
	SA2HSoUnit.strSoName = szSoName;
	SA2HSoUnit.pHandle = pHandle;
	SA2HSoUnit.pFunCreate = create_unit;
	SA2HSoUnit.pFunDestroy = destroy_unit;

	sm_mapSo.insert(make_pair(szSoName,SA2HSoUnit));

	return 0;
}


int CReqAvenue2HttpTransfer::CreateTransferObjs()
{
	TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s\n",__FUNCTION__);

	map<string,SA2HSoUnit>::iterator iter;
	for(iter = sm_mapSo.begin(); iter != sm_mapSo.end(); ++iter)
	{
		string strSoName = iter->first;
		/* if m_mapUnitBySoName has the SoName, don't create class instance */
		map<string,SA2HTransferUnit>::iterator iterMap = m_mapUnitBySoName.find(strSoName);
		if(iterMap == m_mapUnitBySoName.end())
		{				
			create_a2h_obj create_unit = iter->second.pFunCreate;
			destroy_a2h_obj destroy_unit = iter->second.pFunDestroy;
			
			IRequestAvenueHttpTransfer* pTransferObj = create_unit();
	        if(pTransferObj == NULL)
	        {
	            TS_XLOG(XLOG_ERROR,"CReqAvenue2HttpTransfer::%s, create object failed, SoName[%s]\n",__FUNCTION__,strSoName.c_str());
	            continue;
	        }
			
			string strTemp = pTransferObj->GetPluginSoInfo();
			TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, GetPluginSoInfo(), [%s]\n",__FUNCTION__, strTemp.c_str());
			m_mapSoInfoBySoName.insert(make_pair(strSoName,strTemp));

			SA2HTransferUnit oSTUnit(strSoName,pTransferObj, destroy_unit);			

			vector<string> vecTemp;
			pTransferObj->GetServiceMessageId(vecTemp);
			vector<string>::iterator iterTemp;
			for(iterTemp=vecTemp.begin();iterTemp!=vecTemp.end();++iterTemp)
			{
				TS_XLOG(XLOG_TRACE,"CReqAvenue2HttpTransfer::%s, srvMsgId[%s]\n",__FUNCTION__, (*iterTemp).c_str());
				vector<string> vecSplit;
				boost::algorithm::split( vecSplit, (*iterTemp), boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);  
				if( vecSplit.size() == 2 )
				{
					SServiceMsgId msgIdentify(atoi(vecSplit[0].c_str()),atoi(vecSplit[1].c_str()));
					oSTUnit.vecSrvMsgId.push_back(msgIdentify);
					m_mapTransferUnit.insert(make_pair(msgIdentify,oSTUnit));
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

int CReqAvenue2HttpTransfer::UnLoadTransferObj(const string &strDirPath,const vector<string> &vecSo)
{
	TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, vecSo.size[%d]\n",__FUNCTION__,vecSo.size());

	boost::unique_lock<boost::mutex> lock(sm_mut);
	sm_strReloadErrorMsg = "";
	int nCount = 0;

	vector<string>::const_iterator iterVec;
	for(iterVec = vecSo.begin(); iterVec != vecSo.end(); ++iterVec)
	{
		string strSoName = *iterVec;
		string strFullPath = strDirPath + CST_PATH_SEP + strSoName;
		map<string,SA2HTransferUnit>::iterator iterMap = m_mapUnitBySoName.find(strFullPath);
		if(iterMap != m_mapUnitBySoName.end())
		{	
			/*destroy the instance of so*/
			SA2HTransferUnit &sUnit = iterMap->second;
			TS_XLOG(XLOG_TRACE,"CReqAvenue2HttpTransfer::%s, m_mapUnitBySoName.erase so_map[%s]\n",__FUNCTION__,sUnit.strSoName.c_str());	
			destroy_a2h_obj pFunDestroy = sUnit.pFunDestroy;
			pFunDestroy(sUnit.pTransferObj);
			m_mapUnitBySoName.erase(iterMap);

			/*erase the m_mapTransferUnit*/
			vector<SServiceMsgId>::iterator itrSrv;
			for(itrSrv = sUnit.vecSrvMsgId.begin(); itrSrv != sUnit.vecSrvMsgId.end(); ++itrSrv)
			{
				m_mapTransferUnit.erase(*itrSrv);
			}	
		}
		else
		{			
			TS_XLOG(XLOG_WARNING,"CReqAvenue2HttpTransfer::%s, There is no such so in m_mapUnitBySoName[%s]\n",__FUNCTION__,strFullPath.c_str());
		}			
		
		sm_nReloadResult = 0;
		map<string,SA2HSoUnit>::iterator iterSmMap = sm_mapSo.find(strFullPath);
		if(iterSmMap != sm_mapSo.end())
		{		
			iterSmMap->second.nLoadNum--;
			TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, soName[%s],sm_nLoadNum[%d]\n",__FUNCTION__,strFullPath.c_str(),iterSmMap->second.nLoadNum);
			if(iterSmMap->second.nLoadNum == 0)
			{
				TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s,dlcolse so[%s]\n",__FUNCTION__,strFullPath.c_str());
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
			TS_XLOG(XLOG_WARNING,"CReqAvenue2HttpTransfer::%s, There is no such so in sm_mapSo[%s]\n",__FUNCTION__,strSoName.c_str());
			sm_nReloadResult = -2;
			sm_strReloadErrorMsg += string("so[") + strSoName + "] hasn't been loaded;";
			nCount++;
		}
	}

	/* true if in the last thread, notify the condition singal*/
	if(nCount == (int)vecSo.size())
	{
		TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, In the last thread, notify\n",__FUNCTION__);
		sm_cond.notify_one();
	}
	return 0;
}

int CReqAvenue2HttpTransfer::ReLoadTransferObj(const string & strDirPath, const vector<string> &vecSoName)
{	
	TS_XLOG(XLOG_TRACE,"CReqAvenue2HttpTransfer::%s\n",__FUNCTION__);
	
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
			TS_XLOG(XLOG_TRACE,"CReqAvenue2HttpTransfer::%s, szTransferObj[%s]\n",__FUNCTION__,szTransferObj);
			map<string,SA2HSoUnit>::iterator iterSmMap = sm_mapSo.find(szTransferObj);
			if(iterSmMap == sm_mapSo.end())
			{
				if(0 != InstallSoPlugin(szTransferObj))
				{
					TS_XLOG(XLOG_WARNING,"CReqAvenue2HttpTransfer::%s, instatll szTransferObj[%s] failed\n",__FUNCTION__,szTransferObj);
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
				TS_XLOG(XLOG_WARNING,"CReqAvenue2HttpTransfer::%s, szTransferObj[%s] haven't been unloaded\n",__FUNCTION__,szTransferObj);
				sm_strReloadErrorMsg += string(szTransferObj) + " haven't been unloaded;";
			}
		}	
		sm_bReloadFlag = false;
	}

	CreateTransferObjs();

	sm_cond.notify_one();

	return nRet;
}

string CReqAvenue2HttpTransfer::GetPluginSoInfo()
{
	TS_XLOG(XLOG_TRACE,"CReqAvenue2HttpTransfer::%s\n",__FUNCTION__);

	string strPluginSoInfo = "";
	map<string,string>::iterator iter;
	for(iter = m_mapSoInfoBySoName.begin(); iter != m_mapSoInfoBySoName.end();++iter)
	{
		strPluginSoInfo += iter->second;
	}
	return strPluginSoInfo;
}

int CReqAvenue2HttpTransfer::WaitForReload(string &strReloadErorMsg)
{
	TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s\n",__FUNCTION__);
	boost::unique_lock<boost::mutex> lock(sm_mut);   	
	sm_cond.timed_wait<boost::posix_time::seconds>(lock, boost::posix_time::seconds(5));
	
	strReloadErorMsg = sm_strReloadErrorMsg;

	TS_XLOG(XLOG_TRACE,"CReqAvenue2HttpTransfer::%s, sm_nReloadResult[%d], strReloadErorMsg[%s]\n",
		__FUNCTION__,sm_nReloadResult,sm_strReloadErrorMsg.c_str());
	return sm_nReloadResult;
}


int CReqAvenue2HttpTransfer::RequestTransferAvenue2Http(IN unsigned int dwServiceId,IN unsigned int dwMsgId, IN const void * pAvenuePacket, IN int pAvenueLen, IN const char* szRemoteIp, IN const unsigned int dwRemotePort,
    	OUT string &strHttpUrl, OUT void * pHttpReqBody,OUT int *pHttpBodyLen, OUT string &strHttpMethod, IN const void *pInReserve, IN int nInReverseLen, OUT void* pOutReverse, OUT int *pOutReverseLen)
{
	TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, serviceId[%u], msgId[%u], remoteIp[%s], remotePort[%u]\n",__FUNCTION__,dwServiceId,dwMsgId,szRemoteIp,dwRemotePort);

	SServiceMsgId msgIndentify(dwServiceId,dwMsgId);
	map<SServiceMsgId, SA2HTransferUnit>::iterator itrTUnit = m_mapTransferUnit.find(msgIndentify);
    if(itrTUnit != m_mapTransferUnit.end())
    {
    	SA2HTransferUnit& rTUnit = itrTUnit->second;		
		IRequestAvenueHttpTransfer *pTransferObj = rTUnit.pTransferObj;
		
		int ret = pTransferObj->RequestTransferAvenue2Http(pAvenuePacket, pAvenueLen,szRemoteIp, dwRemotePort, 
				strHttpUrl,pHttpReqBody,pHttpBodyLen,strHttpMethod,pInReserve,nInReverseLen,pOutReverse,pOutReverseLen);
		
		TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, shared Transfer method: ret[%d]\n",__FUNCTION__,ret);
        if(0 != ret)
    	{			
			TS_XLOG(XLOG_ERROR,"CReqAvenue2HttpTransfer::%s, serviceId[%u], msgId[%u], remoteIp[%s], remotePort[%u], ret[%d]\n",__FUNCTION__,dwServiceId,dwMsgId,szRemoteIp,dwRemotePort,ret);
			return ERROR_TRANSFER_REQUEST_FAILED;
        }
    }
    else
    {
		TS_XLOG(XLOG_ERROR,"CReqAvenue2HttpTransfer::%s, cannot find transger so.  serviceId[%u], msgId[%u], remoteIp[%s], remotePort[%u]\n",__FUNCTION__,dwServiceId,dwMsgId,szRemoteIp,dwRemotePort);
        return ERROR_TRANSFER_NOT_FOUND;
    } 

	return 0;	
}

int CReqAvenue2HttpTransfer::ResponseTransferHttp2Avenue(IN unsigned int dwServiceId, IN unsigned int dwMsgId, IN unsigned int dwSequence,IN int nAvenueCode,IN int nHttpCode, IN const string &strHttpResponseBody,
    	OUT void * pAvenueResponsePacket, OUT int *pAvenueLen, IN const void *pInReserve, IN int nInReverseLen, OUT void* pOutReverse, OUT int *pOutReverseLen)
{
	TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, serviceId[%u], msgId[%u],pHttpResponseBody[%s]\n",__FUNCTION__,dwServiceId,dwMsgId,strHttpResponseBody.c_str());

	SServiceMsgId msgIndentify(dwServiceId,dwMsgId);
	map<SServiceMsgId, SA2HTransferUnit>::iterator itrTUnit = m_mapTransferUnit.find(msgIndentify);
    if(itrTUnit != m_mapTransferUnit.end())
    {
		IRequestAvenueHttpTransfer *pTransferObj = itrTUnit->second.pTransferObj;
		int nRet = pTransferObj->ResponseTransferHttp2Avenue(dwServiceId, dwMsgId, dwSequence,nAvenueCode,nHttpCode,strHttpResponseBody,
					pAvenueResponsePacket,pAvenueLen,pInReserve,nInReverseLen,pOutReverse,pOutReverseLen);
        if(0 != nRet)
    	{
			TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, ResponseTransferHttp2Avenue fail.  serviceId[%u], msgId[%u],pHttpResponseBody[%s], nRet[%d]\n",
					__FUNCTION__, dwServiceId,dwMsgId,strHttpResponseBody.c_str(),nRet);
			return ERROR_TRANSFER_RESPONSE_FAILED;
        }
    }
    else
    {
		TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s, cannot find so.  serviceId[%u], msgId[%u],pHttpResponseBody[%s\n",
				__FUNCTION__,dwServiceId,dwMsgId,strHttpResponseBody.c_str());
        return ERROR_TRANSFER_NOT_FOUND;
    } 

	return 0;
}


void CReqAvenue2HttpTransfer::CleanRubbish()
{
	TS_XLOG(XLOG_DEBUG,"CReqAvenue2HttpTransfer::%s\n",__FUNCTION__);
	map<string,SA2HTransferUnit>::iterator iter;
	for(iter = m_mapUnitBySoName.begin(); iter != m_mapUnitBySoName.end(); ++iter)
	{		
		SA2HTransferUnit& rTUnit = iter->second;
		IRequestAvenueHttpTransfer *pTransferObj = rTUnit.pTransferObj;
		pTransferObj->CleanRubbish();
	}
}

void CReqAvenue2HttpTransfer::Dump()
{
	TS_XLOG(XLOG_NOTICE,"============CReqAvenue2HttpTransfer::%s ==m_mapUnitBySoName.size[%u]======\n",__FUNCTION__,m_mapUnitBySoName.size());
	map<string,SA2HTransferUnit>::iterator itr = m_mapUnitBySoName.begin();  
	for(;itr!=m_mapUnitBySoName.end();++itr)
	{
		
		TS_XLOG(XLOG_NOTICE,"          <%s, %0X, vecSrvMsgId.size[%u]>\n",itr->second.strSoName.c_str(),itr->second.pTransferObj,itr->second.vecSrvMsgId.size());
	}

	TS_XLOG(XLOG_NOTICE,"  +m_mapTransferUnit.size[%u]===========\n",m_mapTransferUnit.size());
	map<SServiceMsgId, SA2HTransferUnit>::iterator iter;
	for(iter = m_mapTransferUnit.begin(); iter != m_mapTransferUnit.end(); ++iter)
	{
		TS_XLOG(XLOG_NOTICE,"          <%d-%d, %s, %0X>\n",iter->first.dwServiceID,iter->first.dwMsgID, iter->second.strSoName.c_str(),iter->second.pTransferObj);
	}

	TS_XLOG(XLOG_NOTICE,"============CReqAvenue2HttpTransfer::%s  end==============\n",__FUNCTION__);
}



