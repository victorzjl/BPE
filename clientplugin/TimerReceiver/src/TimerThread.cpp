#include "TimerThread.h"
#include "AsyncVirtualServiceLog.h"
#include "TimerAsync.h"
#include "SapMessage.h"
#include "SapTLVBody.h"
#include <netinet/in.h>
#ifdef WIN32
#include <objbase.h>
#else
#include <uuid/uuid.h>
#endif

using namespace sdo::sap;

unsigned int CTimerThread::m_seq = 1;

void createUUID(char *guid, size_t len)
{
    uuid_t uu;
    #ifdef WIN32
    if (S_OK == ::CoCreateGuid(&uu))
    {
    snprintf(guid, len,"%X%X%X%02X%02X%02X%02X%02X%02X%02X%02X",
        uu.Data1,uu.Data2,uu.Data3,uu.Data4[0],uu.Data4[1],uu.Data4[2],uu.Data4[3],uu.Data4[4],uu.Data4[5],uu.Data4[6],uu.Data4[7]);
    }
    #else
        uuid_generate( uu );
        snprintf(guid, len, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                uu[0], uu[1], uu[2],  uu[3],  uu[4],  uu[5],  uu[6],  uu[7],
                uu[8], uu[9], uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
    #endif
}

/*
void createUUID2(char *guid, size_t len)
{
    boost::uuids::uuid_t uu;
    snprintf(guid, len, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                uu[0], uu[1], uu[2],  uu[3],  uu[4],  uu[5],  uu[6],  uu[7],
                uu[8], uu[9], uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
}
*/


CTimerThread* CTimerThread::GetInstance()
{
    static CTimerThread inst;
    return &inst;
}


CTimerThread::CTimerThread()
{

}

CTimerThread::~CTimerThread()
{

}

void CTimerThread::Start()
{
    m_thread = boost::thread(boost::bind(&CTimerThread::Run,this));
}

void CTimerThread::Stop()
{
    this->m_thread.interrupt();
    this->m_thread.join();
}

bool CTimerThread::isAlive()
{
    bool bResult = bRun;
    bRun = false;
    return bResult;
}

void CTimerThread::Run()
{
    SV_XLOG(XLOG_DEBUG,"CTimerThread::%s, ----------begin----------.\n",__FUNCTION__);

    char guid[64] = {0};
    while(1){
        bRun = true;

        sleep(1);

        struct timeval_a now;
        gettimeofday_a(&now,0);

        for(vector<SConfigRequest>::iterator iter=m_vecRequest.begin();iter!=m_vecRequest.end();iter++)
        {
            SConfigRequest &oRequest = *iter;

            struct timeval_a timeDiff;
            timersub(&now, &oRequest.tmStart, &timeDiff);
            if(timeDiff.tv_sec >= (int)oRequest.nInterval)
            {
                oRequest.tmStart.tv_sec = now.tv_sec;
                oRequest.tmStart.tv_usec = now.tv_usec;


                createUUID(guid, sizeof(guid));

                SSapMsgHeader *pHeader=(SSapMsgHeader *)oRequest.pPacket;
                pHeader->dwSequence=htonl(m_seq++);

                memcpy((unsigned char *)pHeader+sizeof(SSapMsgHeader)+4,guid,32);

                CTimerAsync::GetInstance()->gVRequest(CTimerAsync::GetInstance(), (void*)guid, pHeader, oRequest.nPacketLen );

                //dump_sap_packet_info(oRequest.pPacket);

                SV_XLOG(XLOG_DEBUG,"CTimerThread::%s, Send Request ServiceId[%d] MessageId[%d] Guid[%s] Seq[%d].\n",__FUNCTION__,oRequest.nServiceId,oRequest.nMessageId,guid,m_seq);

                sleep(1);
            }else{
                SV_XLOG(XLOG_TRACE,"CTimerThread::%s, ServiceId[%d] MessageId[%d] Waiting[%d].\n",__FUNCTION__,oRequest.nServiceId,oRequest.nMessageId,timeDiff.tv_sec);
            }
        }
    }
}



void CTimerThread::dump_sap_packet_info(const void *pBuffer)
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

