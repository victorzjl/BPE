#include "SessionSos.h"
#include "SessionManager.h"
#include "SapMessage.h"
#include "SapCommon.h"
#include "SapAgent.h"
#include "TestLogHelper.h"
#include "Cipher.h"
#include "SapAdminCommand.h"

using namespace sdo::sap;
string CSessionSos::sm_strUser="admin";
string CSessionSos::sm_strPass="admin";
static int nServerReceive=0;
static int nServerResponse=0;

CSessionSos::CSessionSos(int nId,const map<int,SSapCommandResponseTest *> &mapResponse):
	m_nId(nId),m_mapResponse(mapResponse),m_bAesTransfer(false),m_type(SAP_SERVER_SPS)
{
	CCipher::CreateRandBytes(m_szAesKey,16);
}


void CSessionSos::OnReceiveMsg(void *pBuffer, int nLen)
{
    TEST_XLOG(XLOG_ERROR,"OnReceiveMsg %d\n",++nServerReceive);
    if(nServerReceive==1)
        TEST_XLOG(XLOG_FATAL,"OnReceiveMsg %d\n",nServerReceive);
    
    if(DoAdminRequest_(pBuffer,nLen)==0)
        return;
    
	SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
	if(pHeader->byIdentifer==SAP_PACKET_RESPONSE)
		return;
				
	CSapDecoder request(pBuffer,nLen);
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
			if(attrCur.m_nAttriId==SAP_Attri_nonce&&
					strncasecmp(attrCur.m_strValue.c_str(),"auto",4)==0)
			{
				CCipher::CreateRandBytes(m_szNonce,16);
					response.SetValue(attrCur.m_nAttriId,m_szNonce,16);
			}
			else if(attrCur.m_nAttriId==SAP_Attri_nonce&&
					strncasecmp(attrCur.m_strValue.c_str(),"auto",4)!=0)
			{
				response.SetValue(attrCur.m_nAttriId,attrCur.m_strValue);
			}
			else if(attrCur.m_nAttriId==SAP_Attri_encKey&&
					strncasecmp(attrCur.m_strValue.c_str(),"auto",4)==0)
			{
				unsigned char szEncKey[16];
					unsigned char szPass[16];
					memset(szPass,0,16);
					strncpy((char *)szPass,m_strPass.c_str(),16);
					CCipher::AesEnc(szPass,(unsigned char *)m_szAesKey,16,szEncKey);
					response.SetValue(attrCur.m_nAttriId,szEncKey,16);
			}
			else if(attrCur.m_nAttriId==SAP_Attri_encKey&&
					strncasecmp(attrCur.m_strValue.c_str(),"auto",4)!=0)
			{
				response.SetValue(attrCur.m_nAttriId,attrCur.m_strValue);
			}
			else if(attrCur.m_nAttriId==SAP_Attri_addr)
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
    TEST_XLOG(XLOG_ERROR,"ResponseMsg %d\n",++nServerResponse);
    if(nServerResponse%100000==0)
        TEST_XLOG(XLOG_FATAL,"ResponseMsg %d\n",nServerResponse);
	CSapAgent::Instance()->DoSendMessage(m_nId,response.GetBuffer(),response.GetLen());
	if(m_bAesTransfer==false && m_type==SAP_SERVER_SOC)
		m_bAesTransfer=true;
}
void CSessionSos::OnSendMsg(const SSapCommandTest * pCmd)
{
    CSapEncoder msg(SAP_PACKET_REQUEST,pCmd->m_nServiceId,pCmd->m_nMsgId);
    msg.CreateAndSetSequence();
    vector<SSapAttributeTest>::const_iterator itr;
    for(itr=pCmd->m_vecAttri.begin();itr!=pCmd->m_vecAttri.end();itr++)
    {
        SSapAttributeTest attrCur=*itr;
        if(attrCur.m_nAttriId==SAP_Attri_type)
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

void CSessionSos::Dump()const
{
	TEST_XLOG(XLOG_INFO,"    id[%d]\n",m_nId);
}
int CSessionSos::DoAdminRequest_(void *pBuffer,int nLen)
{
    return -1;
}

