#include "SocSession.h"
#include "SapLogHelper.h"
#include "Cipher.h"
#include "SapStack.h"
#include "AsyncLogThread.h"
#include "AsyncLogHelper.h"

CSocSession::CSocSession(unsigned int dwIndex,const string &strAddr,SapConnection_ptr ptrConn,CSessionManager *pManager):
    ITransferObj("CSocSession",SOC_AUDIT_MODULE,dwIndex,strAddr,pManager),IComposeBaseObj("CSocSession",pManager,SOS_AUDIT_MODULE),
    m_isSuper(false),m_dwId(dwIndex),m_pManager(pManager),m_ptrConn(ptrConn),m_state(E_SOC_UNREGISTED),
    isCompletedGetConfig(false),isGetConfig(false),
    m_tmConnected(m_pManager,30,boost::bind(&CSocSession::Close,this),CThreadTimer::TIMER_ONCE),m_exHeadLen(0)
{}

void CSocSession::Init(bool isSuper)
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s,isSuper[%d]\n",__FUNCTION__,isSuper);
    m_ptrConn->SetOwner(this);

    m_peerAddr=inet_addr(m_ptrConn->GetRemoteIp().c_str());
	m_netPort=htonl(m_ptrConn->GetRemotePort());

	m_isSuper = isSuper;
    m_tmConnected.Start();
    if(isSuper||CSapStack::isAuthSoc==0||CSapStack::isAsc!=0)
    {
        OnRegisted(m_strAddr,E_SUPER_SOC);
    }
}

int CSocSession::OnRegisted(const string & strName,ESocType logintype)
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s\n",__FUNCTION__);
    
    m_dwLoginType=logintype;
    if(m_pManager->OnRegistedSoc(this)==0)//register sucessfully
    {
        m_strUserName=strName;
        m_state=E_SOC_REGISTED;
        m_tmConnected.Stop();
        m_pManager->OnReportRegisterSoc(this);

		if (CSapStack::isAsc==0)
		{
			//add soc name, route info
			SSapMsgAttribute *pAttiName=(SSapMsgAttribute *)(m_exHeader+m_exHeadLen);
			pAttiName->wType=htons(1);
			int nValueLen=m_strUserName.length();
			unsigned int nFactLen=((nValueLen&0x03)!=0?((nValueLen>>2)+1)<<2:nValueLen);//four bytes alignment
			pAttiName->wLength=htons(nValueLen+sizeof(SSapMsgAttribute));
			memcpy(pAttiName->acValue,m_strUserName.c_str(),nValueLen);
			memset(pAttiName->acValue+nValueLen,0,nFactLen-nValueLen);
			m_exHeadLen += nFactLen+sizeof(SSapMsgAttribute);
		}

		SSapMsgAttribute *pAttiRoute=(SSapMsgAttribute *)(m_exHeader+m_exHeadLen);
		pAttiRoute->wType=htons(2);
		pAttiRoute->wLength=htons(8+sizeof(SSapMsgAttribute));
		memcpy(pAttiRoute->acValue,&m_peerAddr,4);
		memcpy(pAttiRoute->acValue+4,&m_netPort,4);

		m_exHeadLen += 8+sizeof(SSapMsgAttribute);
       
        return 0;
    }
    else
    {
        return -1;
    }
}

CSocSession::~CSocSession()
{
    SS_XLOG(XLOG_TRACE,"CSocSession::%s\n",__FUNCTION__);

    m_ptrConn->Close();
    m_ptrConn->SetOwner(NULL);
    m_tmConnected.Stop();
    m_ptrConn.reset();
}

void CSocSession::Close()
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s\n",__FUNCTION__);
    m_pManager->OnDeleteSoc(this);
}

void CSocSession::OnPeerClose()
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s\n",__FUNCTION__);
    m_pManager->OnDeleteSoc(this);
}

void CSocSession::DoDeleteSos(CSosSession * pSos)
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s,pSos[%x]\n",__FUNCTION__,pSos);
	map<unsigned int ,CSosSession *>::iterator itr;
	for(itr=m_mapLastSos.begin();itr!=m_mapLastSos.end();)
	{
		map<unsigned int ,CSosSession *>::iterator itrTmp=itr++;
		if(itrTmp->second==pSos)
		{
			m_mapLastSos.erase(itrTmp);
		}
    }
}

void CSocSession::OnReadCompleted(const void *pBuffer, int nLen)
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s,buffer[%x],nlen[%d]\n",__FUNCTION__,pBuffer,nLen);
    
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
     
    if(pHead->byIdentifer==SAP_PACKET_REQUEST)
    {
        unsigned int dwServiceId=ntohl(pHead->dwServiceId);
        unsigned int dwMsgId=ntohl(pHead->dwMsgId);
        if(dwServiceId==SAP_SMS_SERVICE_ID&&dwMsgId==SMS_GET_CONFIG)
        {
            OnReceiveGetConfig(pBuffer,nLen);
            return;
        }
        if(dwServiceId==SAP_PLATFORM_SERVICE_ID)
        {
            if(dwMsgId==SAP_MSG_Register)
            {
                if(m_state!=E_SOC_REGISTED)
                {
                    OnReceiveRegister(pBuffer,nLen);
                }
                else
                {
                    FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_SUCC);
                }
            }
            else
            {
                FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_NOT_SUPPORT);
            }
        }              
        else
        {
            OnReceiveOtherRequest(pBuffer, nLen);
        }
    }
    else 
    {
        OnTransferResponse(pBuffer, nLen);
    }
}

void CSocSession::OnReceiveGetConfig(const void * pBuffer, int nLen)
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s,buffer[%x],nlen[%d]\n",__FUNCTION__,pBuffer,nLen);
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwServiceId=ntohl(pHead->dwServiceId);
    unsigned int dwMsgId=ntohl(pHead->dwMsgId);

    isGetConfig=true;
    m_seqGetconfig=ntohl(pHead->dwSequence);
    
    CSapDecoder decode(pBuffer,nLen);
    decode.DecodeBodyAsTLV();
    
    string strIp;
    unsigned int dwPort;
    if(decode.GetValue(SMS_Attri_ip,strIp)==0||decode.GetValue(SMS_Attri_port,&dwPort)==0)
    {
        SS_XLOG(XLOG_WARNING,"CSocSession::%s,can't put ip & port in packet\n",__FUNCTION__);
        FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_BAD_REQUEST);
        return;
    }

    string strUserName;
    if(decode.GetValue(SMS_Attri_userName,strUserName)==0)
    {
        m_pManager->GetSocConfig(m_dwId,strUserName,m_ptrConn->GetRemoteIp(),m_ptrConn->GetRemotePort());
    }
    else
    {
        m_pManager->GetSocConfig(m_dwId,m_ptrConn->GetRemoteIp(),m_ptrConn->GetRemotePort());
    }
}

void CSocSession::OnReceiveRegister(const void * pBuffer, int nLen)
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s,buffer[%x],nlen[%d]\n",__FUNCTION__,pBuffer,nLen);
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwServiceId=ntohl(pHead->dwServiceId);
    unsigned int dwMsgId=ntohl(pHead->dwMsgId);

    m_dwLastSeq=ntohl(pHead->dwSequence);
    
    CSapDecoder decode(pBuffer,nLen);
    decode.DecodeBodyAsTLV();

    decode.GetValue(SAP_Attri_version,m_strVersion);
    decode.GetValue(SAP_Attri_buildTime,m_strBuildTime);

    string strUserName;
    bool bWithName=true;
    if(decode.GetValue(SAP_Attri_userName,strUserName)!=0)
    {
        strUserName=m_strAddr;
        bWithName=false;
    }

    if(CSapStack::isAuthSoc==0)
    {
        if(OnRegisted(strUserName,E_UNREGISTED_SOC)==0)
        {
            FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_SUCC);
        }
        else
        {
            FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_HAVE_LOGIN);
        }
        return;
    }
    
    // need authorize
    void *pData;
    unsigned int pLen;
    if(decode.GetValue(SAP_Attri_digestReponse,&pData,&pLen)!=0)
    {
        m_state=E_SOC_RECEIVED_REGISTER;
        m_strUserName=strUserName;
		
        if(isCompletedGetConfig==false)
        {
            if(bWithName)
                m_pManager->GetSocConfig(m_dwId,strUserName);
            else
                m_pManager->GetSocConfig(m_dwId,m_ptrConn->GetRemoteIp(),m_ptrConn->GetRemotePort());
        }
        
        CSapEncoder registerResponse;
        
        registerResponse.SetService(SAP_PACKET_RESPONSE,SAP_PLATFORM_SERVICE_ID,SAP_MSG_Register);
        registerResponse.SetSequence(ntohl(pHead->dwSequence));
        registerResponse.SetCode(SAP_CODE_UNAUTH);
        
        CCipher::CreateRandBytes(m_arNounce,16);
        registerResponse.SetValue(SAP_Attri_nonce,m_arNounce,16);
        m_pManager->RecordRequest(m_strAddr,SAP_PLATFORM_SERVICE_ID,SAP_MSG_Register,SAP_CODE_UNAUTH);
        m_ptrConn->WriteData(registerResponse.GetBuffer(),registerResponse.GetLen());
    }
    else if(m_state==E_SOC_RECEIVED_REGISTER)//reregister
    {
        m_state=E_SOC_RECEIVED_REREGISTER;
        if(strUserName!=m_strUserName||pLen!=16)
        {
            SS_XLOG(XLOG_WARNING,"CSocSession::%s,name[%s:%s],len[%d:16], can't match\n",__FUNCTION__,
                strUserName.c_str(),m_strUserName.c_str(),pLen);
            FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_REJECT);
            return ;
        }
        memcpy(m_arDegest,pData,16);
        DetectIsResponse();
    }
    else
    {
        FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_STATE_ERROR);
    }
}

void CSocSession::OnReceiveOtherRequest(const void * pBuffer, int nLen)
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s,buffer[%x],nlen[%d]\n",__FUNCTION__,pBuffer,nLen);
    SSapMsgHeader *pHead=(SSapMsgHeader *)pBuffer;
    unsigned int dwServiceId=ntohl(pHead->dwServiceId);
    unsigned int dwMsgId=ntohl(pHead->dwMsgId);
    if(pHead->byHeadLen>nLen||nLen>MAX_EXT_SOC_BUFFER)
    {
        SS_XLOG(XLOG_WARNING,"CSocSession::%s,headlen[%d],len[%d], bad request\n",__FUNCTION__,
                pHead->byHeadLen,nLen);
        FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_BAD_REQUEST);
        return;
    }
    
    /*extern header must have no username*/
    void * pExHead=NULL;
    unsigned int nExHead=0;
    if(dwServiceId!=SAP_SLS_SERVICE_ID&&pHead->byHeadLen>sizeof(SSapMsgHeader))
    {
        pExHead=(char *)pBuffer+sizeof(SSapMsgHeader);
        nExHead=pHead->byHeadLen-sizeof(SSapMsgHeader);
    }
    else if(dwServiceId==SAP_SLS_SERVICE_ID && pHead->byHeadLen > (sizeof(SSapMsgHeader)+32))
    {
        pExHead=(char *)pBuffer+sizeof(SSapMsgHeader)+32;
        nExHead=pHead->byHeadLen-sizeof(SSapMsgHeader)-32;
    }
 
	if(CSapStack::isAsc==0 && nExHead>0)
    {
        CSapTLVBodyDecoder decoder(pExHead,nExHead);
        string strName;        
        if(decoder.GetValue(1,strName)==0)
        {
            FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_BAD_REQUEST);
            return;
        }
    }
    
    if(CSapStack::isAuthSoc==0 || CSapStack::isAsc!=0 || m_isSuper ||
		(m_state==E_SOC_REGISTED &&
			(CSapStack::isLocalPrivilege==0?
				m_privilege.IsOperate(dwServiceId, dwMsgId): 
				m_pManager->IsCanRequest(m_ptrConn->GetRemoteIp(),dwServiceId,dwMsgId))))
    {        
		/*isContext request, 0 single, 1 context start, 2 context end*/     
        unsigned char cContext=pHead->byContext;
        if(cContext==0x0)
        {
            if(ProcessComposeRequest(pBuffer,nLen,m_exHeader,m_exHeadLen,
                m_ptrConn->GetRemoteIp(), m_ptrConn->GetRemotePort())==-1 &&
                ProcessDataComposeRequest(pBuffer,nLen,m_exHeader,m_exHeadLen,
                m_ptrConn->GetRemoteIp(), m_ptrConn->GetRemotePort())==-1)
            {
                m_pManager->OnReceiveRequest(m_ptrConn,pBuffer,nLen,m_exHeader,m_exHeadLen);
            }
        }
        else if(cContext==0x1)
        {
            CSosSession *pSos=NULL;
			map<unsigned int ,CSosSession *>::iterator itr=m_mapLastSos.find(dwServiceId);
			if(itr!=m_mapLastSos.end())
            {
				pSos=itr->second;
				if(pSos->State()==CSosSession::E_SOS_CONNECTED)
				{
					pSos->OnTransferRequest(SAP_Request_Remote,m_ptrConn,pBuffer,nLen,m_exHeader,m_exHeadLen);
				}
				else
				{
					if((pSos=m_pManager->OnReceiveRequest(m_ptrConn,pBuffer,nLen,m_exHeader,m_exHeadLen))!=NULL)
					{
						m_mapLastSos[dwServiceId]=pSos;
					}
				}
            }
            else
            {
				if((pSos=m_pManager->OnReceiveRequest(m_ptrConn,pBuffer,nLen,m_exHeader,m_exHeadLen))!=NULL)
				{
					m_mapLastSos[dwServiceId]=pSos;
				}
            }
        }
        else 
        {
            CSosSession *pSos=NULL;
			map<unsigned int ,CSosSession *>::iterator itr=m_mapLastSos.find(dwServiceId);
			if(itr!=m_mapLastSos.end())
            {
				pSos=itr->second;
				if(pSos->State()==CSosSession::E_SOS_CONNECTED)
				{
					pSos->OnTransferRequest(SAP_Request_Remote,m_ptrConn,pBuffer,nLen,m_exHeader,m_exHeadLen);
				}
				else
				{
					pSos=m_pManager->OnReceiveRequest(m_ptrConn,pBuffer,nLen,m_exHeader,m_exHeadLen);
				}
            }
            else
            {
                m_pManager->OnReceiveRequest(m_ptrConn,pBuffer,nLen,m_exHeader,m_exHeadLen);
            }
			m_mapLastSos.erase(dwServiceId);
        }
    }
    else
    {
        FastResponse(dwServiceId,dwMsgId,ntohl(pHead->dwSequence),SAP_CODE_REJECT);
    }
}

void CSocSession::DoGetSocConfigResponse(const void *pBuffer,unsigned int nLen)
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s,extend head len[%d]\n",__FUNCTION__,m_exHeadLen);
    CSapDecoder decode(pBuffer,nLen);
    decode.DecodeBodyAsTLV();

	string strPrivilege;
	decode.GetValue(SMS_Attri_privilige,strPrivilege);
	m_privilege.SetPrivilege(strPrivilege);

	vector<SSapValueNode> vctVelueNode;
	decode.GetValues(SMS_Attri_confValue, vctVelueNode);

	map<unsigned int, string> mapKeyValues;

	//unsigned char *pConfig = NULL;
    //unsigned int nConfigLen = 0;

	for (vector<SSapValueNode>::iterator iter = vctVelueNode.begin(); 
		iter != vctVelueNode.end(); ++iter)
	{
		SKeyValue *pKeyValue = (SKeyValue *)iter->pLoc;
		nLen = iter->nLen;
		if (nLen <= sizeof(SKeyValue))
			continue;

		string str((char*)pKeyValue->szValue, nLen-sizeof(SKeyValue));
		mapKeyValues[htonl(pKeyValue->nKey)] = str;
	}

	map<unsigned int, string>::iterator valueIter;
	unsigned int dwAreaId=0, dwGroupId=0, dwHostId=0,dwAppId=0, dwSpId=0;
	if ((valueIter = mapKeyValues.find(SMS_CfgKey_Ip)) != mapKeyValues.end())
		m_strIp = valueIter->second;
	if ((valueIter = mapKeyValues.find(SMS_CfgKey_Port)) != mapKeyValues.end())
		m_dwPort = atoi(valueIter->second.c_str());
	if ((valueIter = mapKeyValues.find(SMS_CfgKey_SocPwd)) != mapKeyValues.end())
		m_strPass = valueIter->second;
	if ((valueIter = mapKeyValues.find(SMS_CfgKey_SignKey)) != mapKeyValues.end())
		m_strKey = valueIter->second;
	if ((valueIter = mapKeyValues.find(SMS_CfgKey_AppId)) != mapKeyValues.end())
		dwAppId = atoi(valueIter->second.c_str());
	if ((valueIter = mapKeyValues.find(SMS_CfgKey_AreaId)) != mapKeyValues.end())
		dwAreaId = atoi(valueIter->second.c_str());
	if ((valueIter = mapKeyValues.find(SMS_CfgKey_GroupId)) != mapKeyValues.end())
		dwGroupId = atoi(valueIter->second.c_str());
	if ((valueIter = mapKeyValues.find(SMS_CfgKey_HostId)) != mapKeyValues.end())
		dwHostId = atoi(valueIter->second.c_str());
	if ((valueIter = mapKeyValues.find(SMS_CfgKey_SpId)) != mapKeyValues.end())
		dwSpId = atoi(valueIter->second.c_str());

	m_dwAppId = dwAppId;
    m_dwAreaId = dwAreaId;
    m_dwGroupId = dwGroupId;
    m_dwHostId = dwHostId;
    m_dwSpId = dwSpId;

	unsigned int dwLen = 4+sizeof(SSapMsgAttribute);
    SSapMsgAttribute *pAttiAppId=(SSapMsgAttribute *)(m_exHeader+m_exHeadLen);
    pAttiAppId->wType=htons(3);
    pAttiAppId->wLength=htons(dwLen);
    dwAppId = htonl(dwAppId);
    memcpy(pAttiAppId->acValue,&dwAppId,4);

    SSapMsgAttribute *pAttiAreaId=(SSapMsgAttribute *)(m_exHeader+m_exHeadLen+dwLen);
    pAttiAreaId->wType=htons(4);
    pAttiAreaId->wLength=htons(dwLen);
    dwAreaId = htonl(dwAreaId);
    memcpy(pAttiAreaId->acValue,&dwAreaId,4);

    SSapMsgAttribute *pAttiGroupId=(SSapMsgAttribute *)(m_exHeader+m_exHeadLen+2*dwLen);
    pAttiGroupId->wType=htons(5);
    pAttiGroupId->wLength=htons(dwLen);
    dwGroupId = htonl(dwGroupId);
    memcpy(pAttiGroupId->acValue,&dwGroupId,4);

    SSapMsgAttribute *pAttiHostId=(SSapMsgAttribute *)(m_exHeader+m_exHeadLen+3*dwLen);
    pAttiHostId->wType=htons(6);
    pAttiHostId->wLength=htons(dwLen);
    dwHostId = htonl(dwHostId);
    memcpy(pAttiHostId->acValue,&dwHostId,4);

    SSapMsgAttribute *pAttiSpId=(SSapMsgAttribute *)(m_exHeader+m_exHeadLen+4*dwLen);
    pAttiSpId->wType=htons(7);
    pAttiSpId->wLength=htons(dwLen);
    dwSpId = htonl(dwSpId);
    memcpy(pAttiSpId->acValue,&dwSpId,4); 

    m_exHeadLen += 5*dwLen;

    isCompletedGetConfig = true;
    DetectIsResponse();    
    
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s,isGetConfig:%d\n",__FUNCTION__,isGetConfig);
    if(isGetConfig)
    {
		isGetConfig=false;

		char szBuff[1024];
		SKeyValue *keyValue = (SKeyValue*)szBuff;

		CSapEncoder msgResponse;
		msgResponse.SetService(SAP_PACKET_RESPONSE,SAP_SMS_SERVICE_ID,SMS_GET_CONFIG,decode.GetCode());
		msgResponse.SetSequence(m_seqGetconfig);

		keyValue->nKey = htonl(SMS_CfgKey_Ip);
		strcpy((char*)keyValue->szValue, m_strIp.c_str());
		msgResponse.SetValue(SMS_Attri_confValue, (void*)szBuff, strlen((char*)keyValue->szValue)+sizeof(SKeyValue));

		keyValue->nKey = htonl(SMS_CfgKey_Port);
		sprintf((char*)keyValue->szValue, "%d", m_dwPort);
		msgResponse.SetValue(SMS_Attri_confValue, (void*)szBuff, strlen((char*)keyValue->szValue)+sizeof(SKeyValue));

		keyValue->nKey = htonl(SMS_CfgKey_SocPwd);
		strcpy((char*)keyValue->szValue, m_strPass.c_str());
		msgResponse.SetValue(SMS_Attri_confValue, (void*)szBuff, strlen((char*)keyValue->szValue)+sizeof(SKeyValue));

		keyValue->nKey = htonl(SMS_CfgKey_SignKey);
		strcpy((char*)keyValue->szValue, m_strKey.c_str());
		msgResponse.SetValue(SMS_Attri_confValue, (void*)szBuff, strlen((char*)keyValue->szValue)+sizeof(SKeyValue));

		keyValue->nKey = htonl(SMS_CfgKey_AppId);
		sprintf((char*)keyValue->szValue, "%d", m_dwAreaId);
		msgResponse.SetValue(SMS_Attri_confValue, (void*)szBuff, strlen((char*)keyValue->szValue)+sizeof(SKeyValue));

		keyValue->nKey = htonl(SMS_CfgKey_AreaId);
		sprintf((char*)keyValue->szValue, "%d", m_dwAreaId);
		msgResponse.SetValue(SMS_Attri_confValue, (void*)szBuff, strlen((char*)keyValue->szValue)+sizeof(SKeyValue));

		keyValue->nKey = htonl(SMS_CfgKey_GroupId);
		sprintf((char*)keyValue->szValue, "%d", m_dwGroupId);
		msgResponse.SetValue(SMS_Attri_confValue, (void*)szBuff, strlen((char*)keyValue->szValue)+sizeof(SKeyValue));

		keyValue->nKey = htonl(SMS_CfgKey_HostId);
		sprintf((char*)keyValue->szValue, "%d", m_dwHostId);
		msgResponse.SetValue(SMS_Attri_confValue, (void*)szBuff, strlen((char*)keyValue->szValue)+sizeof(SKeyValue));

		keyValue->nKey = htonl(SMS_CfgKey_SpId);
		sprintf((char*)keyValue->szValue, "%d", m_dwSpId);
		msgResponse.SetValue(SMS_Attri_confValue, (void*)szBuff, strlen((char*)keyValue->szValue)+sizeof(SKeyValue));

		/*isGetConfig=false;
        CSapEncoder msgResponse;
        msgResponse.SetService(SAP_PACKET_RESPONSE,SAP_SMS_SERVICE_ID,SMS_GET_CFG_PRIV,decode.GetCode());
        msgResponse.SetSequence(m_seqGetconfig);

        unsigned int nLeft=(0x10-nConfigLen)&0x0f;
        unsigned char *pEncConfig=(unsigned char *)malloc(nConfigLen+nLeft);
        CCipher::AesEnc((unsigned char *)PUBLIC_CONFIG_ENCKEY,pConfig,nConfigLen,pEncConfig);

		char szBuffer[1024];
		SKeyValue *pReq = (SKeyValue*)szBuffer;
		pReq->nKey = htonl(SMS_CfgKey_config);
		memcpy((char *)pReq->szValue, pEncConfig, nConfigLen+nLeft);
				
        msgResponse.SetValue(SMS_Attri_confValue, (void*)pReq, nConfigLen+nLeft+sizeof(SKeyValue));
        free(pEncConfig);

		pReq->nKey = htonl(SMS_CfgKey_isEnc);
		sprintf((char *)pReq->szValue, "1");
        msgResponse.SetValue(SMS_Attri_confValue, strlen((char *)pReq->szValue)+sizeof(SKeyValue));
        */
        m_pManager->RecordRequest(m_strAddr,SAP_SMS_SERVICE_ID,SMS_GET_CFG_PRIV,decode.GetCode());
		m_ptrConn->WriteData(msgResponse.GetBuffer(),msgResponse.GetLen());
    }
}

void CSocSession::DetectIsResponse()
{
    SS_XLOG(XLOG_DEBUG,"CSocSession::%s,state[%d],isCompletedGetConfig[%d]\n",__FUNCTION__,m_state,isCompletedGetConfig);
    CSapEncoder encoderRegisterResponse;
    encoderRegisterResponse.SetService(SAP_PACKET_RESPONSE,SAP_PLATFORM_SERVICE_ID,SAP_MSG_Register);
    encoderRegisterResponse.SetSequence(m_dwLastSeq);
    if(m_state==E_SOC_RECEIVED_REREGISTER&&isCompletedGetConfig)
    {        
	unsigned char arDegest[16];
        string strPassNounce=m_strPass+string((char *)m_arNounce,16);
        
        CCipher::Md5((unsigned char *)strPassNounce.c_str(),strPassNounce.size(),arDegest);

        if(IsEnableLevel(SAP_STACK_MODULE, XLOG_TRACE))
        {
                SS_XLOG(XLOG_TRACE,"       nouce[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
            m_arNounce[0],m_arNounce[1],m_arNounce[2],m_arNounce[3],
            m_arNounce[4],m_arNounce[5],m_arNounce[6],m_arNounce[7],
            m_arNounce[8],m_arNounce[9],m_arNounce[10],m_arNounce[11],
            m_arNounce[12],m_arNounce[13],m_arNounce[14],m_arNounce[15]);

		SS_XLOG(XLOG_TRACE,"       pwd[%s][%d],strPassNounce[%d]\n",
            m_strPass.c_str(),m_strPass.length(),strPassNounce.size());
                
                SS_XLOG(XLOG_TRACE,"       degest1[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
            m_arDegest[0],m_arDegest[1],m_arDegest[2],m_arDegest[3],
            m_arDegest[4],m_arDegest[5],m_arDegest[6],m_arDegest[7],
            m_arDegest[8],m_arDegest[9],m_arDegest[10],m_arDegest[11],
            m_arDegest[12],m_arDegest[13],m_arDegest[14],m_arDegest[15]);

                SS_XLOG(XLOG_TRACE,"       degest2[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
            arDegest[0],arDegest[1],arDegest[2],arDegest[3],
            arDegest[4],arDegest[5],arDegest[6],arDegest[7],
            arDegest[8],arDegest[9],arDegest[10],arDegest[11],
            arDegest[12],arDegest[13],arDegest[14],arDegest[15]);
                SS_XLOG(XLOG_TRACE,"       pass[%s],addr[%s:%d],fact[%s:%d]\n",
                    m_strPass.c_str(),
                    m_strIp.c_str(),m_dwPort,
                    m_ptrConn->GetRemoteIp().c_str(),m_ptrConn->GetRemotePort());
        }
        
        if(memcmp(arDegest,m_arDegest,16)==0
            &&(m_strIp=="0.0.0.0"||m_strIp==m_ptrConn->GetRemoteIp())
            &&(m_dwPort==0||m_dwPort==m_ptrConn->GetRemotePort()))
        {
            if(OnRegisted(m_strUserName,E_REGISTED_SOC)==0)
            {
                encoderRegisterResponse.SetCode(SAP_CODE_SUCC);
                encoderRegisterResponse.SetValue(SAP_Attri_spid,m_dwSpId);
                encoderRegisterResponse.SetValue(SAP_Attri_serverid,m_dwAppId);
                encoderRegisterResponse.SetValue(SAP_Attri_areaid,m_dwAreaId);
                encoderRegisterResponse.SetValue(SAP_Attri_groupid,m_dwGroupId);
                encoderRegisterResponse.SetValue(SAP_Attri_hostid,m_dwHostId);

                if(CSapStack::isVerifySocSignature!=0)
                {
                    m_ptrConn->SetPeerDigest(m_strKey);
    		        encoderRegisterResponse.SetValue(SAP_Attri_IsVerifySignature,1);
                }

                if(CSapStack::isSocEnc!=0)
                {
                    unsigned char m_szEncKey[16];
                    CCipher::CreateRandBytes(m_arAesKey,16);
                    
                    unsigned char szKey[16];
                    memset(szKey,16,0);
                    strncpy((char *)szKey,m_strKey.c_str(),16);

                    CCipher::AesEnc(szKey,m_arAesKey,16,m_szEncKey);
                    encoderRegisterResponse.SetValue(SAP_Attri_encType,1);
                    encoderRegisterResponse.SetValue(SAP_Attri_encKey,m_szEncKey,16);
                    if(IsEnableLevel(SAP_STACK_MODULE, XLOG_TRACE))
                    {
                            SS_XLOG(XLOG_TRACE,"       m_strKey[%s],aeskey[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
                        m_strKey.c_str(),m_arAesKey[0],m_arAesKey[1],m_arAesKey[2],m_arAesKey[3],
                        m_arAesKey[4],m_arAesKey[5],m_arAesKey[6],m_arAesKey[7],
                        m_arAesKey[8],m_arAesKey[9],m_arAesKey[10],m_arAesKey[11],
                        m_arAesKey[12],m_arAesKey[13],m_arAesKey[14],m_arAesKey[15]);
                    }
                }
                m_pManager->RecordRequest(m_strAddr,SAP_PLATFORM_SERVICE_ID,SAP_MSG_Register,SAP_CODE_SUCC);
    	        m_ptrConn->WriteData(encoderRegisterResponse.GetBuffer(),encoderRegisterResponse.GetLen());
				if(CSapStack::isSocEnc!=0)
				{
					m_ptrConn->SetEncKey(m_arAesKey);
				}
            }
            else
            {
                SS_XLOG(XLOG_WARNING,"CSocSession::%s,name[%s] have login\n",__FUNCTION__,m_strUserName.c_str());
                encoderRegisterResponse.SetCode(SAP_CODE_HAVE_LOGIN);
                m_pManager->RecordRequest(m_strAddr,SAP_PLATFORM_SERVICE_ID,SAP_MSG_Register,SAP_CODE_HAVE_LOGIN);
                m_ptrConn->WriteData(encoderRegisterResponse.GetBuffer(),encoderRegisterResponse.GetLen());
            }
        }
        else
        {
            encoderRegisterResponse.SetCode(SAP_CODE_REJECT);
            m_pManager->RecordRequest(m_strAddr,SAP_PLATFORM_SERVICE_ID,SAP_MSG_Register,SAP_CODE_REJECT);
            m_ptrConn->WriteData(encoderRegisterResponse.GetBuffer(),encoderRegisterResponse.GetLen());           
        }
    }
}

void CSocSession::FastResponse(unsigned int dwServiceId,unsigned int dwMsgId,unsigned int dwSequence,unsigned int dwCode)
{
    SSapMsgHeader response;
    response.byIdentifer=SAP_PACKET_RESPONSE;
    response.dwServiceId=htonl(dwServiceId);
    response.dwMsgId=htonl(dwMsgId);
    response.dwSequence=htonl(dwSequence);
    
    response.byContext=0;
	response.byPrio=0;
	response.byBodyType=0;
	response.byCharset=0;

    response.byHeadLen=sizeof(SSapMsgHeader);
    response.dwPacketLen=htonl(sizeof(SSapMsgHeader));
    response.byVersion=0x01;
    response.byM=0xFF;
    response.dwCode=htonl(dwCode);
    memset(response.bySignature,0,16);
    m_pManager->RecordRequest(m_strAddr,dwServiceId,dwMsgId, dwCode);
    m_ptrConn->WriteData(&response,sizeof(SSapMsgHeader));
}

void CSocSession::Dump()
{
    SS_XLOG(XLOG_NOTICE,"#soc#   index[%d],manager[%x],id[%u],addr[%s],seq[%u],state[%d]\n",
        m_pManager,m_dwId,m_dwId,m_strAddr.c_str(),m_seq,m_state);
    SS_XLOG(XLOG_NOTICE,"       isCompletedGetConfig[%d],strname[%s],config[pass:%s key:%s addr:%s:%d],LoginType[%d]\n",
        isCompletedGetConfig,m_strUserName.c_str(),m_strPass.c_str(),m_strKey.c_str(),
        m_strIp.c_str(),m_dwPort,m_dwLoginType);
    SS_XLOG(XLOG_NOTICE,"       nouce[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
        m_arNounce[0],m_arNounce[1],m_arNounce[2],m_arNounce[3],
        m_arNounce[4],m_arNounce[5],m_arNounce[6],m_arNounce[7],
        m_arNounce[8],m_arNounce[9],m_arNounce[10],m_arNounce[11],
        m_arNounce[12],m_arNounce[13],m_arNounce[14],m_arNounce[15]);
    SS_XLOG(XLOG_NOTICE,"       degest[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
        m_arDegest[0],m_arDegest[1],m_arDegest[2],m_arDegest[3],
        m_arDegest[4],m_arDegest[5],m_arDegest[6],m_arDegest[7],
        m_arDegest[8],m_arDegest[9],m_arDegest[10],m_arDegest[11],
        m_arDegest[12],m_arDegest[13],m_arDegest[14],m_arDegest[15]);
    SS_XLOG(XLOG_NOTICE,"       aeskey[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
        m_arAesKey[0],m_arAesKey[1],m_arAesKey[2],m_arAesKey[3],
        m_arAesKey[4],m_arAesKey[5],m_arAesKey[6],m_arAesKey[7],
        m_arAesKey[8],m_arAesKey[9],m_arAesKey[10],m_arAesKey[11],
        m_arAesKey[12],m_arAesKey[13],m_arAesKey[14],m_arAesKey[15]);
    m_privilege.Dump();
    m_ptrConn->Dump();
    IComposeBaseObj::Dump();
}
