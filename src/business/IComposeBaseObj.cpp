#include "ITransferObj.h"
#include "AsyncLogThread.h"
#include "AsyncLogHelper.h"
#include "SessionManager.h"
#include "SapLogHelper.h"
#include "IComposeBaseObj.h"
#include "ComposeServiceObj.h"
#include "ComposeDataServiceObj.h"
#include "SosSessionContainer.h"
#include "ServiceAvailableTrigger.h"


IComposeBaseObj::IComposeBaseObj(const string &strModule,CSessionManager *pManager, int level)
    :m_strModule(strModule),m_pManager(pManager),m_level(level),m_dwComposeIndex(1)
{}

IComposeBaseObj::~IComposeBaseObj()
{  
    SS_XLOG(XLOG_DEBUG,"%s::%s\n",m_strModule.c_str(),__FUNCTION__);
    NotifyOffLine();
    m_pManager->OnAddComposeObjNum(m_mapComposeObj.size());
    m_mapComposeObj.clear();   
}

int IComposeBaseObj::ProcessComposeRequest(const void * pBuffer, int nLen, 
    const void *pExtHead, int nExt,     
    const string& sIp, int port, void* handle)
{
    SS_XLOG(XLOG_DEBUG,"%s::%s len[%d],exlen[%d]\n",m_strModule.c_str(),__FUNCTION__,nLen,nExt);
    SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;
    SComposeKey sKey(ntohl(pHead->dwServiceId), ntohl(pHead->dwMsgId));
    
    const SComposeServiceUnit *pCSUnit = m_pManager->GetComposeUnit(sKey); 
    if(NULL == pCSUnit)
    {
        SS_XLOG(XLOG_DEBUG,"%s::%s, find compose service failed.service[%d],msg[%d]\n",
            m_strModule.c_str(),__FUNCTION__,sKey.dwServiceId, sKey.dwMsgId);   
        return -1;
    } 

    CAvenueServiceConfigs *pConfig = m_pManager->GetServiceConfiger();
    if(NULL == pConfig)
    {
        SS_XLOG(XLOG_WARNING,"%s::%s, load avenue configure failed\n",
            m_strModule.c_str(),__FUNCTION__);   
        return -1;
    }
    CAvailableTrigger oAvail(pConfig, m_pManager);
	if(!oAvail.BeginTrigger(sKey.dwServiceId, sKey.dwMsgId))
	{
		SS_XLOG(XLOG_WARNING,"%s::%s, sub service of compose service[%d],msg[%d] not avaiable \n",
			m_strModule.c_str(),__FUNCTION__,sKey.dwServiceId, sKey.dwMsgId);  
        FastResponse(GetIpAddr(pBuffer, nLen,pExtHead,nExt),pBuffer,nLen,SAP_CODE_SERVICE_UNAVAIABLE,handle);		
		return 0;
	}
    
    CComposeServiceObj *pComposeObj = new CComposeServiceObj(this,*pCSUnit,pConfig,
        m_pManager,ntohl(pHead->dwSequence));     
	SReturnMsgData returnMsg;
	int nCode=pComposeObj->Init(m_dwComposeIndex++,m_level,SSRequestMsg(pExtHead, nExt, SIPAddr(sIp,port)),pBuffer,nLen,handle,returnMsg);
	
	SS_XLOG(XLOG_DEBUG,"IComposeBaseObj::%s returncode[%d] returnMsg[%s]\n",__FUNCTION__,
	returnMsg.nReturnMsgType, returnMsg.strReturnMsg.c_str());
		 
    if(nCode!=0)
    {       
        delete pComposeObj;
        FastResponse(GetIpAddr(pBuffer, nLen,pExtHead,nExt),pBuffer,nLen,nCode,handle,returnMsg);            
    }    

   return 0;    
}

int IComposeBaseObj::ProcessDataComposeRequest(const void * pBuffer, int nLen, 
                                           const void *pExtHead, int nExt,     
                                           const string& sIp, int port)
{
    SS_XLOG(XLOG_DEBUG,"%s::%s len[%d],exlen[%d]\n",m_strModule.c_str(),__FUNCTION__,nLen,nExt);

    SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;
    if (ntohl(pHead->dwServiceId) != DATASERVICE_SERVICEID ||
        ntohl(pHead->dwMsgId) != DATASERVICE_MSGID)
    {
        SS_XLOG(XLOG_DEBUG,"%s::%s, It's not data server message.service[%d],msg[%d]\n",
            m_strModule.c_str(),__FUNCTION__,pHead->dwServiceId, pHead->dwMsgId);   
        return -1;
    }

    SDataMsgRequest sDataMsgRequest;
    sDataMsgRequest.pMsgHead = pHead;
    CSapDecoder reqDecode(pBuffer, nLen);
    reqDecode.DecodeBodyAsTLV();
    reqDecode.GetValue(DATA_ATTR_SERVICE_ID,    &sDataMsgRequest.sComposeKey.dwServiceId);
    reqDecode.GetValue(DATA_ATTR_MSG_ID,        &sDataMsgRequest.sComposeKey.dwMsgId);
    reqDecode.GetValue(DATA_ATTR_RESULT_CODE,   (unsigned*)&sDataMsgRequest.nResultCode);
    reqDecode.GetValue(DATA_ATTR_REQUEST_DATA,  &sDataMsgRequest.sRequestData.pLoc, &sDataMsgRequest.sRequestData.nLen);
    reqDecode.GetValue(DATA_ATTR_RESPONSE_DATA, &sDataMsgRequest.sResponseData.pLoc, &sDataMsgRequest.sResponseData.nLen);
    reqDecode.GetValues(DATA_ATTR_DEF_VALUE,    sDataMsgRequest.vecDefValue);

    const SComposeServiceUnit *pCSUnit = m_pManager->GetComposeUnit(sDataMsgRequest.sComposeKey); 
    if(NULL == pCSUnit || !pCSUnit->isDataCompose)
    {
        SS_XLOG(XLOG_DEBUG, "%s::%s, find data compose service failed.service[%d],msg[%d] unit[%p]\n",
            m_strModule.c_str(), __FUNCTION__,
            sDataMsgRequest.sComposeKey.dwServiceId, sDataMsgRequest.sComposeKey.dwMsgId, pCSUnit);   
        FastDataResponse(GetIpAddr(pBuffer, nLen, pExtHead, nExt), sDataMsgRequest.sComposeKey, pBuffer, nLen, SAP_CODE_NOT_FOUND);
        return 0;
    } 

    CAvenueServiceConfigs *pConfig = m_pManager->GetServiceConfiger();
    if(NULL == pConfig)
    {
        SS_XLOG(XLOG_WARNING,"%s::%s, load avenue configure failed\n",
            m_strModule.c_str(),__FUNCTION__);   
        FastDataResponse(GetIpAddr(pBuffer, nLen, pExtHead, nExt), sDataMsgRequest.sComposeKey, pBuffer, nLen, SAP_CODE_NOT_FOUND);
        return 0;
    }

    CComposeDataServiceObj *pComposeObj = new CComposeDataServiceObj(this, *pCSUnit, pConfig,
        m_pManager,ntohl(pHead->dwSequence));  		
    if(0 != pComposeObj->Init(m_dwComposeIndex++, m_level, SSRequestMsg(pExtHead, nExt, SIPAddr(sIp,port)), sDataMsgRequest))
    {       
        delete pComposeObj;
        FastDataResponse(GetIpAddr(pBuffer, nLen, pExtHead, nExt), sDataMsgRequest.sComposeKey, pBuffer, nLen, SAP_CODE_SERVICE_DECODE_ERROR);
    }

    return 0;
}


void IComposeBaseObj::OnDeleteComposeService(unsigned nComposeObjId)
{
	//SS_XLOG(XLOG_DEBUG,"%s::%s, compose obj Id[%d]\n",m_strModule.c_str(),__FUNCTION__, nComposeObjId);
    m_mapComposeObj.erase(nComposeObjId);
	/*map<unsigned, CComposeServiceObj*>::iterator itr = 
		m_mapComposeObj.find(nComposeObjId);
	if(itr != m_mapComposeObj.end())
	{                       
		m_mapComposeObj.erase(itr); 
	} */
}

void IComposeBaseObj::OnAddComposeService(unsigned dwId, CComposeServiceObj* pCSObj)
{
	//SS_XLOG(XLOG_DEBUG,"%s::%s, compose obj Id[%d]\n",m_strModule.c_str(),__FUNCTION__, dwId);
	m_mapComposeObj.insert(make_pair(dwId,pCSObj));
}

void IComposeBaseObj::FastResponse(const string &strAddr,const void *pBuffer, int nLen, int nCode, void* handle, SReturnMsgData& returnMsg)
{
	SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;
    unsigned dwSeviceId  = ntohl(pHead->dwServiceId);
    unsigned dwMsgId     = ntohl(pHead->dwMsgId);
    unsigned dwSeq       = ntohl(pHead->dwSequence);
    CSapEncoder oResponse(SAP_PACKET_RESPONSE,dwSeviceId,dwMsgId,nCode);
    oResponse.SetSequence(dwSeq);
    m_pManager->RecordBusinessReq(strAddr,pBuffer, nLen,nCode,m_level, true);
	if ((returnMsg.nReturnMsgType!=0) && (returnMsg.strReturnMsg.size()>0))
	{
		oResponse.SetValue(returnMsg.nReturnMsgType,returnMsg.strReturnMsg);
	}
    WriteComposeData(handle,oResponse.GetBuffer(),oResponse.GetLen());
}

void IComposeBaseObj::FastResponse(const string &strAddr,const void *pBuffer, int nLen, int nCode, void* handle)
{
    SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;
    unsigned dwSeviceId  = ntohl(pHead->dwServiceId);
    unsigned dwMsgId     = ntohl(pHead->dwMsgId);
    unsigned dwSeq       = ntohl(pHead->dwSequence);
    CSapEncoder oResponse(SAP_PACKET_RESPONSE,dwSeviceId,dwMsgId,nCode);
    oResponse.SetSequence(dwSeq);
    m_pManager->RecordBusinessReq(strAddr,pBuffer, nLen,nCode,m_level, true);
    WriteComposeData(handle,oResponse.GetBuffer(),oResponse.GetLen());	
}

void IComposeBaseObj::FastDataResponse(const string &strAddr, const SComposeKey& sComposeKey, const void *pBuffer, int nLen, int nCode)
{
    SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;
    unsigned dwSeviceId  = ntohl(pHead->dwServiceId);
    unsigned dwMsgId     = ntohl(pHead->dwMsgId);
    unsigned dwSeq       = ntohl(pHead->dwSequence);
    CSapEncoder oResponse(SAP_PACKET_RESPONSE,dwSeviceId,dwMsgId,nCode);
    oResponse.SetSequence(dwSeq);
    m_pManager->RecordDataReq(strAddr, sComposeKey.dwServiceId, sComposeKey.dwMsgId, nCode);
    WriteComposeData(NULL,oResponse.GetBuffer(),oResponse.GetLen());	
}

void IComposeBaseObj::Dump()
{     
    SS_XLOG(XLOG_NOTICE,"===========IComposeBaseObj::Dump [%d]=========\n",m_mapComposeObj.size());
    map<unsigned, CComposeServiceObj*>::iterator itr;    
    for(itr=m_mapComposeObj.begin();itr!=m_mapComposeObj.end();++itr)
    {
        CComposeServiceObj* pCSObj = itr->second;
        pCSObj->Dump();
    }
}

void IComposeBaseObj::NotifyOffLine()
{
    map<unsigned, CComposeServiceObj*>::iterator itr;    
    for(itr=m_mapComposeObj.begin();itr!=m_mapComposeObj.end();++itr)
    {
        CComposeServiceObj *pObj = itr->second;  
        pObj->NotifyOffLine();  
    }
}

