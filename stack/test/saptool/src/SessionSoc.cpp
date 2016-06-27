#include "SessionSoc.h"
#include "SessionSocFsm.h"
#include "SessionManager.h"
#include "SapMessage.h"
#include "SapCommon.h"
#include "SapAgent.h"
#include "TestLogHelper.h"
#include "Cipher.h"
#include <boost/bind.hpp>
#include	<time.h>
#include	<utility>
using namespace sdo::sap;
static int nSend=0;
static int nResponse=0;
static int m_nMinTime = 100000;
static int m_nMaxTime = 0;

static int n500=0;
static int n1000=0;
static int n2000=0;
static int n5000=0;
static int n8000=0;
static int n10000=0;
static int n15000=0;
static int nlast=0;





CSessionSoc::CSessionSoc(int nId,SSocConfUnit * pConfUnit,const map<int,SSapCommandResponseTest *> &mapResponse)
    :m_nId(nId),m_pConfUnit(pConfUnit),m_mapResponse(mapResponse),
    m_fsm(this,SessionSocStateTable,TEST_UNCONNECTED_STATE,NULL),
   m_timer(CSessionManager::Instance(),0,boost::bind(&CSessionSoc::DoTimeout,this),sdo::common::CThreadTimer::TIMER_ONCE),
    m_bAesTransfer(false),m_nClientType(SAP_SERVER_SPS)
{
    m_itr=m_pConfUnit->m_vecCommands.begin();
    TEST_XLOG(XLOG_DEBUG,"CSessionSoc::%s,commands[%d]\n",__FUNCTION__,m_pConfUnit->m_vecCommands.size());
}
void CSessionSoc::OnSessionTimeout()
{
    TEST_XLOG(XLOG_DEBUG,"CSessionSoc::%s,create[TEST_TIMEOUT_EVENT],id[%d],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
    m_fsm.PostEventRun(TEST_TIMEOUT_EVENT,NULL);
}

void CSessionSoc::OnConnectSucc()
{
    TEST_XLOG(XLOG_DEBUG,"CSessionSoc::%s,create[TEST_CONNECT_EVENT],id[%d],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
    m_fsm.PostEventRun(TEST_CONNECT_EVENT,NULL);
}

void CSessionSoc::OnReceiveMsg(void *pBuffer, int nLen)
{
    
    SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
    if(pHeader->byIdentifer==SAP_PACKET_REQUEST)
    {
        OnRequest_(pBuffer,nLen);
        return;
    }
    
    TEST_XLOG(XLOG_DEBUG,"CSessionSoc::%s,create[TEST_RECEIVE_RESPONSE_EVENT],id[%d],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
    SSessionResponse data;
    data.m_pBuffer=pBuffer;
    data.m_nLen=nLen;

    m_fsm.PostEventRun(TEST_RECEIVE_RESPONSE_EVENT,&data);
    
}
void CSessionSoc::DoTimeout()
{
    //CSessionManager::Instance()->OnSessionTimeout((int)pData);
    m_fsm.PostEventRun(TEST_TIMEOUT_EVENT,NULL);
}

/*event function*/
void CSessionSoc::DoStartTimer(void *pData)
{
    TEST_XLOG(XLOG_TRACE,"CSessionSoc::%s,id[%d],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
    SSapCommandTest cmdCur=*(m_itr-1);
    m_timer.SetInterval(cmdCur.m_nInterval);
    m_timer.Start();
}

void CSessionSoc::DoStopTimer(void *pData)
{
    TEST_XLOG(XLOG_TRACE,"CSessionSoc::%s,id[%d],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
    m_timer.Stop();
}

void CSessionSoc::DoSendRequest(void *pData)
{
    TEST_XLOG(XLOG_DEBUG,"CSessionSoc::%s,itr++\n",__FUNCTION__);
    if(m_itr==m_pConfUnit->m_vecCommands.end()&&
        (m_pConfUnit->m_type==E_EXECUTE_CIRCLE||m_pConfUnit->m_type==E_EXECUTE_ONCE))
    {
        CSapAgent::Instance()->DoClose(m_nId);
        CSessionManager::Instance()->OnPeerClose(m_nId);
        return;
    }
    else if(m_itr==m_pConfUnit->m_vecCommands.end()&&m_pConfUnit->m_type==E_EXECUTE_CONNECTION_CIRCLE)
    {
        m_itr=m_pConfUnit->m_vecCommands.begin();
    }

        
    TEST_XLOG(XLOG_TRACE,"CSessionSoc::%s,id[%d],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
    SSapCommandTest cmdCur=*m_itr;
    for(int i=0;i<cmdCur.m_nTimes;i++)
    {
        CSapEncoder msg(SAP_PACKET_REQUEST,cmdCur.m_nServiceId,cmdCur.m_nMsgId);
        if(cmdCur.m_nServiceId == 3)
        {
            char szName[32];
            memset(szName, 0, 32);
	    strncpy(szName, m_pConfUnit->m_strSocName.c_str(), 32);
            msg.SetExtHeader(szName, 32);
        }
		else
		{
			CSapTLVBodyEncoder xhead;
			if(m_pConfUnit->m_nAppId!=0) xhead.SetValue(3,m_pConfUnit->m_nAppId);
			if(m_pConfUnit->m_nAreaId!=0)xhead.SetValue(4,m_pConfUnit->m_nAreaId);
			msg.SetExtHeader(xhead.GetBuffer(), xhead.GetLen());
		}
		
        int nSequence=msg.CreateAndSetSequence();

        vector<SSapAttributeTest>::iterator itr;
        for(itr=cmdCur.m_vecAttri.begin();itr!=cmdCur.m_vecAttri.end();itr++)
        {
            SSapAttributeTest attrCur=*itr;
            if(cmdCur.m_nServiceId==1&&cmdCur.m_nMsgId==1&&attrCur.m_nAttriId==4&&
                strncasecmp(attrCur.m_strValue.c_str(),"auto",4)==0)
            {
                unsigned char arDegest[16];
                string strPassNounce=m_pConfUnit->m_strPass + string((char *)m_szNonce,16);
                CCipher::Md5((unsigned char *)strPassNounce.c_str(),strPassNounce.size(),arDegest);
                
                TEST_XLOG(XLOG_TRACE,"nouce[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
        	m_szNonce[0],m_szNonce[1],m_szNonce[2],m_szNonce[3],
        	m_szNonce[4],m_szNonce[5],m_szNonce[6],m_szNonce[7],
       		m_szNonce[8],m_szNonce[9],m_szNonce[10],m_szNonce[11],
        	m_szNonce[12],m_szNonce[13],m_szNonce[14],m_szNonce[15]);
                
                msg.SetValue(attrCur.m_nAttriId,arDegest,16);
            }
            else if(attrCur.m_nType==E_ATTRI_VALUE_INT)
            {
                //TEST_XLOG(XLOG_DEBUG,"SendRequest, SetAttri[%d], int_config[%d]\n",attrCur.m_nAttriId,attrCur.m_nValue);
                msg.SetValue(attrCur.m_nAttriId,attrCur.m_nValue);
            }
            else if(attrCur.m_nType==E_ATTRI_VALUE_STR)
            {
                //TEST_XLOG(XLOG_DEBUG,"SendRequest, SetAttri[%d], str_config[%s]\n",attrCur.m_nAttriId,attrCur.m_strValue.c_str());
                msg.SetValue(attrCur.m_nAttriId,attrCur.m_strValue);
            }
            else if(attrCur.m_nType==E_ATTRI_VALUE_STRUCT)
            {
                if(attrCur.vecStruct.size()>=2)
                {
                    char szValue[256]={0};
                    *((int *)szValue)=htonl(atoi(attrCur.vecStruct[0].c_str()));
                    strcpy(szValue+4,attrCur.vecStruct[1].c_str());
                    
                    msg.SetValue(attrCur.m_nAttriId,szValue,4+strlen(attrCur.vecStruct[1].c_str()));
                }
            }
            
            if(attrCur.m_nAttriId==SAP_Attri_sessionKey)
            {
                m_nClientType=SAP_SERVER_SPS;
            }
        }
        if(m_bAesTransfer)
        {
            TEST_XLOG(XLOG_DEBUG,"SendRequest,Aes Enc\n");
            msg.AesEnc(m_szAesKey);
        }
        else
        {
            TEST_XLOG(XLOG_DEBUG,"SendRequest, No Enc\n");
        }
		
        TEST_XLOG(XLOG_ERROR,"SendRequest,%d\n",++nSend);
        if(nSend==1)
		{
			m_nMinTime = 100000;
			m_nMaxTime = 0;
			TEST_XLOG(XLOG_FATAL,"SendRequest,%d\n",nSend);
		}
                
        CSapAgent::Instance()->DoSendMessage(m_nId,msg.GetBuffer(),msg.GetLen());
		struct timeval now;
		gettimeofday(&now,0);
		SSequenceTime stSequenceTime;
		stSequenceTime.nSequence = nSequence;
		stSequenceTime.timeSend.seconds = now.tv_sec;
		stSequenceTime.timeSend.useconds = now.tv_usec;
        m_vecSequence.push_back(stSequenceTime);
		
		/*
		struct timeval now;
		gettimeofday(&now,0);
		SSapStartTime StartTime;
		StartTime.seconds = now.tv_sec;
		StartTime.seconds = now.tv_usec;
		
		m_mapStarttime.insert(std::make_pair(nSequence,StartTime));
		*/
		
		
    }
    if(m_vecSequence.size()==0)
    {
        SSapCommandTest cmdNext=*m_itr;
        if(cmdNext.m_nInterval<=0)
        {
            TEST_XLOG(XLOG_TRACE,"CSessionSoc::%s,id[%d],create[TEST_COMPLETED_TIMER0_EVENT],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
            m_fsm.PostEvent(TEST_COMPLETED_TIMER0_EVENT,NULL);
        }
        else
        {
            TEST_XLOG(XLOG_TRACE,"CSessionSoc::%s,id[%d],create[TEST_COMPLETED_TIMER_EVENT],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
            m_fsm.PostEvent(TEST_COMPLETED_TIMER_EVENT,NULL);
        }
    }
    m_itr++;
}

void CSessionSoc::DoSaveResponse(void *pData)
{
    TEST_XLOG(XLOG_TRACE,"CSessionSoc::%s,id[%d],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
	
    SSessionResponse *pRspData=(SSessionResponse *)pData;
    SSapMsgHeader *pHeader=(SSapMsgHeader *)pRspData->m_pBuffer;
    CSapDecoder msgDecode(pRspData->m_pBuffer,pRspData->m_nLen);
    msgDecode.DecodeBodyAsTLV();
    
    if(msgDecode.GetServiceId()==1&&
        msgDecode.GetMsgId()==1&&
        msgDecode.GetCode()==-10242401)
    {   
        void *pNonce;
        unsigned int len;
        if(msgDecode.GetValue(3,&pNonce,&len)==0&&
            len==16)
        {
            memcpy(m_szNonce,pNonce,len);
            TEST_XLOG(XLOG_TRACE,"before nouce[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
        	m_szNonce[0],m_szNonce[1],m_szNonce[2],m_szNonce[3],
        	m_szNonce[4],m_szNonce[5],m_szNonce[6],m_szNonce[7],
       		m_szNonce[8],m_szNonce[9],m_szNonce[10],m_szNonce[11],
        	m_szNonce[12],m_szNonce[13],m_szNonce[14],m_szNonce[15]);
        }
    }
    else if(msgDecode.GetServiceId()==1&&
        msgDecode.GetMsgId()==1&&
        msgDecode.GetCode()==0)
    {
        unsigned int dwValue;
        string strKey;
        if(msgDecode.GetValue(5, &dwValue) == 0&&
            msgDecode.GetValue(6, strKey) == 0) 
        {
            unsigned char szEncodeKey[16];
            string & strPwd =m_pConfUnit->m_strKey;
    		CCipher::AesDec((unsigned char *)strPwd.c_str(), (const unsigned char *)strKey.c_str(), 
    			strKey.size(),szEncodeKey);
            
            GetSapAgentInstance()->SetEncKey(m_nId,szEncodeKey);
    		GetSapAgentInstance()->SetLocalDigest(m_nId, m_pConfUnit->m_strKey);
    	}
    }
    int nSequence=msgDecode.GetSequence();
	
	/*
	struct timeval now;
	gettimeofday(&now,0);
	SSapStartTime EndTime;
	EndTime.seconds = now.tv_sec;
	EndTime.seconds = now.tv_usec;
	
	map<int,SSapStartTime>::iterator itTime;
	itTime = m_mapStarttime.find(nSequence);
	
	if( itTime != m_mapStarttime.end() )
	{
		SSapStartTime StartTime;
		StartTime.seconds = (*itTime).second.seconds;
		StartTime.useconds = (*itTime).second.useconds;
		
		TEST_XLOG(XLOG_FATAL,"Sequence Num:%d,%d,%d\n",nSequence,EndTime.seconds-StartTime.seconds,EndTime.seconds-EndTime.seconds);
	}
	
	m_mapStarttime.erase(itTime);
	*/
	
    vector<SSequenceTime>::iterator itr;
    for(itr=m_vecSequence.begin();itr!=m_vecSequence.end();++itr)
    {
		SSequenceTime &stSequenceTime =*itr;
        if(nSequence== stSequenceTime.nSequence)
        {
			struct timeval now;
			gettimeofday(&now,0);
			int nTimePass = (now.tv_sec - stSequenceTime.timeSend.seconds) * 1000000 + (now.tv_usec - stSequenceTime.timeSend.useconds);
            if(nTimePass<=500)
            {
                n500++;
            }
            else if(nTimePass<=1000)
            {
                n1000++;
            }
            else if(nTimePass<=2000)
            {
                n2000++;
            }
            else if(nTimePass<=5000)
            {
                n5000++;
            }
            else if(nTimePass<=8000)
            {
                n8000++;
            }
            else if(nTimePass<=10000)
            {
                n10000++;
            }
            else if(nTimePass<=15000)
            {
                n15000++;
            }
            else
            {
                nlast++;
            }
			m_nMinTime = (nTimePass < m_nMinTime)?nTimePass:m_nMinTime;
			m_nMaxTime = (nTimePass > m_nMaxTime)?nTimePass:m_nMaxTime;
            TEST_XLOG(XLOG_ERROR,"ReceiveResponse,%d\n",++nResponse);
            if(nResponse%10000==0)
			{
                TEST_XLOG(XLOG_FATAL,"ReceiveResponse,%d, m_nMinTime[%d]useconds, m_nMaxTime[%d]useconds\n",nResponse, m_nMinTime, m_nMaxTime);
				m_nMinTime = 100000;
				m_nMaxTime = 0;
			}
            if(nResponse%100000==0)
            {
                TEST_XLOG(XLOG_FATAL,"================100000 static begin==========\n");
                TEST_XLOG(XLOG_FATAL,"  [0~0.5ms] %f%%\n",(float)n500/1000);
                TEST_XLOG(XLOG_FATAL,"  [0.5~1ms] %f%%\n",(float)n1000/1000);
                TEST_XLOG(XLOG_FATAL,"  [1~2ms] %f%%\n",(float)n2000/1000);
                TEST_XLOG(XLOG_FATAL,"  [2~5ms] %f%%\n",(float)n5000/1000);
                TEST_XLOG(XLOG_FATAL,"  [5~8ms] %f%%\n",(float)n8000/1000);
                TEST_XLOG(XLOG_FATAL,"  [8~10ms] %f%%\n",(float)n10000/1000);
                TEST_XLOG(XLOG_FATAL,"  [10~15ms] %f%%\n",(float)n15000/1000);
                TEST_XLOG(XLOG_FATAL,"  [15~..ms] %f%%\n",(float)nlast/1000);
                TEST_XLOG(XLOG_FATAL,"================100000 static end==========\n");
                n500=0;
                n1000=0;
                n2000=0;
                n5000=0;
                n8000=0;
                n10000=0;
                n15000=0;
                nlast=0;
            }
            m_vecSequence.erase(itr);
            break;
        }
    }
    if(m_vecSequence.size()==0)
    {
	    SSapCommandTest cmdNext=*(m_itr-1);
	    if(cmdNext.m_nInterval==-1)
	    {
		    m_itr--;
	    }
        if(cmdNext.m_nInterval<=0)
        {
            TEST_XLOG(XLOG_TRACE,"CSessionSoc::%s,id[%d],create[TEST_COMPLETED_TIMER0_EVENT],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
            m_fsm.PostEvent(TEST_COMPLETED_TIMER0_EVENT,NULL);
        }
        else
        {
            TEST_XLOG(XLOG_TRACE,"CSessionSoc::%s,id[%d],create[TEST_COMPLETED_TIMER_EVENT],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
            m_fsm.PostEvent(TEST_COMPLETED_TIMER_EVENT,NULL);
        }
    }
}

void CSessionSoc::DoDefault(void *pData)
{
    TEST_XLOG(XLOG_TRACE,"CSessionSoc::%s,id[%d],state[%s]\n",__FUNCTION__,m_nId,m_fsm.GetStateName());
}

void CSessionSoc::Dump()const
{
    TEST_XLOG(XLOG_INFO,"    id[%d],cmdNum[%d],time_state[%d],seqNum[%d],fsm[%s]\n",
        m_nId,m_pConfUnit->m_vecCommands.size(),m_timer.State(),m_vecSequence.size(),m_fsm.GetStateName());

}

void CSessionSoc::OnRequest_(void *pData, int nLen)
{				
	CSapDecoder request(pData,nLen);
	if(m_bAesTransfer)
	{
		request.AesDec(m_szAesKey);
	}

	CSapEncoder response(SAP_PACKET_RESPONSE,request.GetServiceId(),request.GetMsgId());
    response.SetSequence(request.GetSequence());

	SSapCommandResponseTest *pConfig=NULL;
	map<int,SSapCommandResponseTest *>::const_iterator itr=m_mapResponse.find((request.GetServiceId()<<8)+request.GetMsgId());
	if(itr!=m_mapResponse.end())
	{
		pConfig=itr->second;
	}
	if(pConfig==NULL)
	{
		response.SetService(SAP_PACKET_RESPONSE,request.GetServiceId(),request.GetMsgId(),44);
	}
	else
	{
        response.SetService(SAP_PACKET_RESPONSE,request.GetServiceId(),request.GetMsgId(),pConfig->m_byCode);
		vector<SSapAttributeTest>::iterator itr;
		for(itr=pConfig->m_vecAttri.begin();itr!=pConfig->m_vecAttri.end();itr++)
		{
			SSapAttributeTest attrCur=*itr;

			if(attrCur.m_nAttriId==SAP_Attri_addr)
			{
				int pos=attrCur.m_strValue.find(':');
				if(pos!=string::npos)
				{
					SSapMsgAttriAddr addr;
					addr.dwIp=inet_addr(attrCur.m_strValue.substr(0,pos).c_str());
					addr.wPort=htons(atoi(attrCur.m_strValue.substr(pos+1).c_str()));
					response.SetValue(attrCur.m_nAttriId,&addr,sizeof(addr));
				}
				
			}
			else if(attrCur.m_nType==E_ATTRI_VALUE_INT)
			{
				response.SetValue(attrCur.m_nAttriId,attrCur.m_nValue);
			}
			else if(attrCur.m_nType==E_ATTRI_VALUE_STR)
            {
                //TEST_XLOG(XLOG_DEBUG,"SendRequest, SetAttri[%d], str_config[%s]\n",attrCur.m_nAttriId,attrCur.m_strValue.c_str());
                response.SetValue(attrCur.m_nAttriId,attrCur.m_strValue);
            }
            else if(attrCur.m_nType==E_ATTRI_VALUE_STRUCT)
            {
                if(attrCur.vecStruct.size()>=2)
                {
                    char szValue[256]={0};
                    *((int *)szValue)=htonl(atoi(attrCur.vecStruct[0].c_str()));
                    strcpy(szValue+4,attrCur.vecStruct[1].c_str());
                    
                    response.SetValue(attrCur.m_nAttriId,szValue,4+strlen(attrCur.vecStruct[1].c_str()));
                }
            }
            
		}
    }
	if(m_bAesTransfer)
	{
		response.AesEnc(m_szAesKey);
	}

	CSapAgent::Instance()->DoSendMessage(m_nId,response.GetBuffer(),response.GetLen());
}
void CSessionSoc::OnSendMsg(const SSapCommandTest * pCmd)
{
    CSapEncoder msg(SAP_PACKET_REQUEST,pCmd->m_nServiceId,pCmd->m_nMsgId);
    msg.CreateAndSetSequence();

    vector<SSapAttributeTest>::const_iterator itr;
    for(itr=pCmd->m_vecAttri.begin();itr!=pCmd->m_vecAttri.end();itr++)
    {
        SSapAttributeTest attrCur=*itr;
        if(attrCur.m_nAttriId==SAP_Attri_digestReponse&&
            strncasecmp(attrCur.m_strValue.c_str(),"auto",4)==0)
        {
            unsigned char szOrigi[m_pConfUnit->m_strPass.length()+16];
            strncpy((char *)szOrigi,m_pConfUnit->m_strPass.c_str(),m_pConfUnit->m_strPass.length());
            memcpy(szOrigi+m_pConfUnit->m_strPass.length(),m_szNonce,16);
            unsigned char szMd5[16];
            CCipher::Md5(szOrigi,sizeof(szOrigi),szMd5);
            msg.SetValue(attrCur.m_nAttriId,szMd5,16);
        }
        else if(attrCur.m_nAttriId==SAP_Attri_digestReponse&&
            strncasecmp(attrCur.m_strValue.c_str(),"auto",4)!=0)
        {
            msg.SetValue(attrCur.m_nAttriId,attrCur.m_strValue);
        }
        else if(attrCur.m_nAttriId==SAP_Attri_type)
        {
            int nClientType=CSapInterpreter::GetTypeValue(attrCur.m_strValue);
            msg.SetValue(attrCur.m_nAttriId,nClientType);
        }
        else if(attrCur.m_nAttriId==SAP_Attri_addr)
        {
            int pos=attrCur.m_strValue.find(':');
            if(pos!=string::npos)
            {
                SSapMsgAttriAddr addr;
                addr.dwIp=inet_addr(attrCur.m_strValue.substr(0,pos).c_str());
                addr.wPort=htons(atoi(attrCur.m_strValue.substr(pos+1).c_str()));
                msg.SetValue(attrCur.m_nAttriId,&addr,sizeof(addr));
            }
            
        }
        else if(attrCur.m_nType==E_ATTRI_VALUE_INT)
        {
            msg.SetValue(attrCur.m_nAttriId,attrCur.m_nValue);
        }
        else
        {
            msg.SetValue(attrCur.m_nAttriId,attrCur.m_strValue);
        }
    }
    if(m_bAesTransfer)
    {
        msg.AesEnc(m_szAesKey);
    }

    CSapAgent::Instance()->DoSendMessage(m_nId,msg.GetBuffer(),msg.GetLen());
}


