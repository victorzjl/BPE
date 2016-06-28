#include "TimerServiceConfig.h"
#include "AsyncVirtualServiceLog.h"
#include <stdio.h>
#include <netinet/in.h>
#include <boost/algorithm/string.hpp>
#include "XmlConfigParser.h"

using namespace sdo::common;

CTimerServiceConfig::CTimerServiceConfig()
{

}

CTimerServiceConfig::~CTimerServiceConfig()
{

}

int CTimerServiceConfig::loadServiceConfig(const string &strConfig)
{
    CXmlConfigParser oConfig;
	if (oConfig.ParseFile(strConfig)!=0)
	{
		SV_XLOG(XLOG_ERROR, "CTimerServiceConfig::%s, parse config error: %s\n", __FUNCTION__, oConfig.GetErrorMessage().c_str());
		return -1;
	}

	SV_XLOG(XLOG_DEBUG, "CTimerServiceConfig::%s, parsing start.\n", __FUNCTION__);

    vector<string> vecReqStri=oConfig.GetParameters("request");
    vector<string>::iterator itrReqStr;
    for(itrReqStr=vecReqStri.begin();itrReqStr!=vecReqStri.end();++itrReqStr)
    {
        string strDetail=*itrReqStr;
        CXmlConfigParser oDetailConfig;
        oDetailConfig.ParseDetailBuffer(strDetail.c_str());

		string strServIdMsgId=oDetailConfig.GetParameter("requestservice","0");

        unsigned int nServiceId=0, nMessageId=0;

        if(2 != sscanf(strServIdMsgId.c_str(), "%d_%d", &nServiceId, &nMessageId))
        {
            continue;
        }

        int itvl=oDetailConfig.GetParameter("interval",3600);

        SV_XLOG(XLOG_DEBUG,"CTimerServiceConfig::%s, nServiceId[%d] nMessageId[%d] Interval[%d].\n", __FUNCTION__,nServiceId,nMessageId,itvl);

        CSapEncoder  msgEncode(SAP_PACKET_REQUEST,nServiceId,nMessageId,0);
        msgEncode.SetSequence(1);
        CSapTLVBodyEncoder exHead;
        exHead.SetValue(9, "12345678901234567890123456789012");
        msgEncode.SetExtHeader(exHead.GetBuffer(),(unsigned int)exHead.GetLen());
        CSapTLVBodyEncoder bodyEncode;

        vector<string> vecParam=oDetailConfig.GetParameters("requestparam");
        for(vector<string>::iterator itrParam=vecParam.begin();itrParam!=vecParam.end();++itrParam)
        {
            string strPara = *itrParam;
            unsigned int nCode=0;
            char szValue[256]={0};
            if(2 == sscanf(strPara.c_str(), "%d,&quot;%[^&]&quot;", &nCode, szValue))
            {
                bodyEncode.SetValue(nCode,string(szValue));
                SV_XLOG(XLOG_DEBUG,"CTimerServiceConfig::%s, nServiceId[%d] nMessageId[%d] StringPara[%d][%s].\n", __FUNCTION__,nServiceId,nMessageId,nCode,szValue);
            }else if (2 == sscanf(strPara.c_str(), "%d,%s", &nCode, szValue))
            {
                bodyEncode.SetValue(nCode,atoi(szValue));
                SV_XLOG(XLOG_DEBUG,"CTimerServiceConfig::%s, nServiceId[%d] nMessageId[%d] IntPara[%d][%s].\n", __FUNCTION__,nServiceId,nMessageId,nCode,szValue);
            }
        }

        msgEncode.SetBody(bodyEncode.GetBuffer(),bodyEncode.GetLen());

        char* pPacket = new char[msgEncode.GetLen()];
        memcpy(pPacket, msgEncode.GetBuffer(), msgEncode.GetLen());

        SConfigRequest stRequest;
        stRequest.nServiceId = nServiceId;
        stRequest.nMessageId = nMessageId;
        stRequest.nInterval = itvl;
        stRequest.pPacket = (void *)pPacket;
        stRequest.nPacketLen = msgEncode.GetLen();
        gettimeofday_a(&stRequest.tmStart,0);

        //dump_sap_packet_info(stRequest.pPacket);

        m_vecRequest.push_back(stRequest);
    }

    return 0;
}


string CTimerServiceConfig::GetInnerTextByElement(TiXmlElement *pElement, const string& defaultValue)
{
	if(pElement == NULL)
	{
		return defaultValue;
	}
	sdo::util::TiXmlPrinter print;
    pElement->InnerAccept(&print);
    string result=print.Str();
    //result=result.substr(0,result.find_last_not_of("   \n\r\t")+1);
    return result==""?defaultValue:result;
}

int CTimerServiceConfig::GetInnerTextByElement(TiXmlElement *pElement, int defaultValue)
{
	if(pElement == NULL)
	{
		return defaultValue;
	}
	sdo::util::TiXmlPrinter print;
    pElement->InnerAccept(&print);
    string result=print.Str();
    //result=result.substr(0,result.find_last_not_of("   \n\r\t")+1);
    boost::trim_right_if(result,boost::is_any_of("   \n\r\t"));
    return result==""?defaultValue:atoi(result.c_str());
}

void CTimerServiceConfig::Dump() const
{
    SV_XLOG(XLOG_NOTICE, "############begin dump############\n");

    for(vector<SConfigRequest>::const_iterator iter=m_vecRequest.begin();iter!=m_vecRequest.end();iter++)
    {
        const SConfigRequest & oConfigRequest = *iter;
        SV_XLOG(XLOG_DEBUG, "nServiceId[%d] nMessageId[%d] Interval[%d].\n",
			oConfigRequest.nServiceId, oConfigRequest.nMessageId, oConfigRequest.nInterval);
    }

    SV_XLOG(XLOG_NOTICE, "############end dump############\n");
}

void CTimerServiceConfig::dump_sap_packet_info(const void *pBuffer)
{

        string strPacket;
        char szPeer[64]={0};
        char szId[16]={0};
        sprintf(szId,"%d",123);
        sprintf(szPeer,"%s:%d","127.0.0.1" ,1000);


        strPacket=string("Write Sap Command from id[")+szId+"] addr["+szPeer+"]\n";

        SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;

        unsigned char *pChar=(unsigned char *)pBuffer;
        unsigned int nPacketlen=ntohl(pHeader->dwPacketLen);
        int nLine=nPacketlen>>3;
        int nLast=(nPacketlen&0x7);
        int i=0;
        for(i=0;i<nLine;i++)
        {
            char szBuf[128]={0};
            unsigned char * pBase=pChar+(i<<3);
            sprintf(szBuf,"                [%2d]    %02x %02x %02x %02x    %02x %02x %02x %02x    ......\n",
                i,*(pBase),*(pBase+1),*(pBase+2),*(pBase+3),*(pBase+4),*(pBase+5),*(pBase+6),*(pBase+7));
            strPacket+=szBuf;
        }

        unsigned char * pBase=pChar+(i<<3);
        if(nLast>0)
        {
            for(int j=0;j<8;j++)
            {
                char szBuf[128]={0};
                if(j==0)
                {
                    sprintf(szBuf,"                [%2d]    %02x ",i,*(pBase+j));
                    strPacket+=szBuf;
                }
                else if(j<nLast&&j==3)
                {
                    sprintf(szBuf,"%02x    ",*(pBase+j));
                    strPacket+=szBuf;
                }
                else if(j>=nLast&&j==3)
                {
                    strPacket+="      ";
                }
                else if(j<nLast)
                {
                    sprintf(szBuf,"%02x ",*(pBase+j));
                    strPacket+=szBuf;
                }
                else
                {
                    strPacket+="   ";
                }

            }
            strPacket+="   ......\n";
        }
        SV_XLOG(XLOG_INFO,strPacket.c_str());

}
