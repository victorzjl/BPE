#include "SapConnection.h"
#include <boost/bind.hpp>
#include <iostream>
#include "SapLogHelper.h"
#include "SapMessage.h"
#include "SapCommon.h"
#include "Cipher.h"
#include "TimerManager.h"
#include "SessionManager.h"


#define MAX_SAP_PAKET_LEN 1024*1024*2

int CSapConnection::sm_nHeartInterval=60;
int CSapConnection::sm_nTimeoutInterval=120;

CSapConnection::CSapConnection(boost::asio::io_service &io_service,void *pManager):
    m_socket(io_service),m_resolver(io_service),
    bClientType(false),m_tmHeartBeat((CTimerManager *)pManager),
    bRunning(true),m_nQueueLen(0),m_pManager(pManager),m_pOwner(NULL),
    m_isLocalDigest(false),m_isPeerDigest(false),m_isEnc(false)
{
    m_heartSequence=0;

    m_heartRequest.byIdentifer=SAP_PACKET_REQUEST;
    m_heartRequest.dwServiceId=0;
    m_heartRequest.dwMsgId=0;
    
    m_heartRequest.byContext=0;
	m_heartRequest.byPrio=0;
	m_heartRequest.byBodyType=0;
	m_heartRequest.byCharset=0;

    m_heartRequest.byHeadLen=sizeof(SSapMsgHeader);
    m_heartRequest.dwPacketLen=htonl(sizeof(SSapMsgHeader));
    m_heartRequest.byVersion=0x01;
    m_heartRequest.byM=0xFF;
    m_heartRequest.dwCode=htonl(SAP_CODE_SUCC);
    memset(m_heartRequest.bySignature,0,16);


    m_heartResponse.byIdentifer=SAP_PACKET_RESPONSE;
    m_heartResponse.dwServiceId=0;
    m_heartResponse.dwMsgId=0;
    
    m_heartResponse.byContext=0;
	m_heartResponse.byPrio=0;
	m_heartResponse.byBodyType=0;
	m_heartResponse.byCharset=0;

    m_heartResponse.byHeadLen=sizeof(SSapMsgHeader);
    m_heartResponse.dwPacketLen=htonl(sizeof(SSapMsgHeader));
    m_heartResponse.byVersion=0x01;
    m_heartResponse.byM=0xFF;
    m_heartResponse.dwCode=htonl(SAP_CODE_SUCC);
    memset(m_heartResponse.bySignature,0,16);
    
}

CSapConnection::~CSapConnection()
{
    SS_XLOG(XLOG_TRACE,"CSapConnection::~CSapConnection,\n");
}

void CSapConnection::AsyncConnect(const string &strIp,unsigned int dwPort,int nTimeout,unsigned int dwLocalPort)
{
    SS_XLOG(XLOG_TRACE,"CSapConnection::async_connect,addr[%s:%d],timeout[%d]\n",strIp.c_str(),dwPort,nTimeout);
    bClientType=true;
    m_tmHeartBeat.Init((CTimerManager *)m_pManager,sm_nHeartInterval,boost::bind(&CSapConnection::OnTimer,shared_from_this()),CThreadTimer::TIMER_CIRCLE);
    
    char szBuf[16]={0};
    sprintf(szBuf,"%d",dwPort);
    tcp::resolver::query query(strIp,szBuf);
    m_resolver.async_resolve(query,
        MakeSapAllocHandler(m_allocReader,
        boost::bind(&CSapConnection::HandleResolve, shared_from_this(),
              boost::asio::placeholders::error,
              boost::asio::placeholders::iterator,dwLocalPort)));
}
void CSapConnection::HandleResolve(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator,unsigned int dwLocalPort)
{
    if (!err)
    {
        if(dwLocalPort!=0)
        {
            boost::system::error_code ec;
            m_socket.open(tcp::v4(),ec);
            if(ec)
            {
                SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",open socket error:" <<ec.message()<<"\n");
                ConnectFinish(-1);
                return;
            }
			m_socket.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true),ec);
            m_socket.bind(tcp::endpoint(tcp::v4(),dwLocalPort),ec);
            if(ec)
            {
                SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",bind port error:" <<ec.message()<<"\n");
                ConnectFinish(-1);
                return;
            }
        }
        tcp::endpoint endpoint = *endpoint_iterator;
        SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,addr[%s:%d]\n",__FUNCTION__,endpoint.address().to_string() .c_str() ,endpoint.port());
        m_socket.async_connect(endpoint,
            MakeSapAllocHandler(m_allocReader,
          boost::bind(&CSapConnection::HandleConnected, shared_from_this(),
            boost::asio::placeholders::error, ++endpoint_iterator)));
    }
    else
    {
      SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",error:" <<err.message()<<"\n");
      ConnectFinish(-2);
    }
}
void CSapConnection::HandleConnected(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s\n",__FUNCTION__);
    if (!err)
    {
        SetRemoteAddr();
        ConnectFinish(0);
        AsyncRead();
    }
    else if (endpoint_iterator != tcp::resolver::iterator())
    {
      boost::system::error_code ignore_ec;
      m_socket.close(ignore_ec);
      tcp::endpoint endpoint = *endpoint_iterator;
      m_socket.async_connect(endpoint,
          MakeSapAllocHandler(m_allocReader,boost::bind(&CSapConnection::HandleConnected, shared_from_this(),
            boost::asio::placeholders::error, ++endpoint_iterator)));
    }
    else
    {
        SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",error:" <<err.message()<<"\n");
        ConnectFinish(-3);
    }
}
void CSapConnection::ConnectFinish(int result)
{
    boost::system::error_code ignore_ec;
    if(m_pOwner!=NULL)
    {
        m_pOwner->OnConnectResult(result);
    }
    if(result!=0)
    {
        CloseInner();
    }
}
void CSapConnection::ASyncReadInner()
{
    ((CSessionManager*)m_pManager)->gettimeofday(&m_AlivePoint, false);
    m_tmHeartBeat.Init((CTimerManager *)m_pManager,sm_nHeartInterval,boost::bind(&CSapConnection::OnTimer,shared_from_this()),CThreadTimer::TIMER_CIRCLE);
    m_tmHeartBeat.Start();
    
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s\n",__FUNCTION__);

    m_pHeader=buffer_.base();
    m_socket.async_read_some( boost::asio::buffer(buffer_.top(),buffer_.capacity()),
					MakeSapAllocHandler(m_allocReader,boost::bind(&CSapConnection::HandleRead, shared_from_this(),
							boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred)));
}

void CSapConnection::SetRemoteAddr()
{
    try{
        m_strRemoteIp=m_socket.remote_endpoint().address().to_string();
		m_dwRemotePort=m_socket.remote_endpoint().port();
    }
    catch(boost::system::system_error & ec){
		SS_SLOG(XLOG_WARNING,"CSapConnection::SetRemoteAddr,code:" <<ec.what()<<"\n");
        m_strRemoteIp="";
        m_dwRemotePort=0;
    }
}
void CSapConnection::AsyncRead()
{
    m_socket.get_io_service().post(boost::bind(&CSapConnection::ASyncReadInner, shared_from_this()));
}
void CSapConnection::HandleRead(const boost::system::error_code& err,std::size_t dwTransferred)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,bytes_transferred[%d]\n",__FUNCTION__,dwTransferred);
    if(!err)
    {
        ((CSessionManager*)m_pManager)->gettimeofday(&m_AlivePoint, false);
        buffer_.inc_loc(dwTransferred);
        while(buffer_.top()>=m_pHeader+sizeof(SSapMsgHeader))
        {
            SSapMsgHeader *ptrHeader=(SSapMsgHeader *)m_pHeader;
            unsigned int packetlen=ntohl(ptrHeader->dwPacketLen);
            if(packetlen<=MAX_SAP_PAKET_LEN&&packetlen>0&&(ptrHeader->byIdentifer==SAP_PACKET_REQUEST||ptrHeader->byIdentifer==SAP_PACKET_RESPONSE))
            {
                if(buffer_.top()>=m_pHeader+packetlen)
                {
                    ReadPacketCompleted(m_pHeader,packetlen);
                    m_pHeader+=packetlen;
                }
                else
                {
                    break;
                }
            }
            else
            {
                SS_XLOG(XLOG_WARNING,"CSapConnection::%s,packetlen[%d],identifier[%d], will close this socket\n",__FUNCTION__,packetlen,ptrHeader->byIdentifer);
                ResponseFail(8,8,ntohl(ptrHeader->dwSequence),SAP_CODE_PACKET_ERROR);
                StopConnection();
                return;
            }
        }
        if(m_pHeader!=buffer_.base())
        {
            memmove(buffer_.base(),m_pHeader,buffer_.top()-m_pHeader);
            buffer_.reset_loc(buffer_.top()-m_pHeader);
            m_pHeader=buffer_.base();
        }
        else if(buffer_.capacity()==0)
        {
			m_pHeader=buffer_.base();
	    	SSapMsgHeader *ptrHeader=(SSapMsgHeader *)m_pHeader;
            unsigned int packetlen=ntohl(ptrHeader->dwPacketLen);
            buffer_.add_capacity(SAP_ALIGN(packetlen));
            m_pHeader=buffer_.base();
        }
        m_socket.async_read_some(boost::asio::buffer(buffer_.top(),buffer_.capacity()),
           MakeSapAllocHandler(m_allocReader,boost::bind(&CSapConnection::HandleRead, shared_from_this(),
                boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred)));
    }
    else
    {
        SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",addr["<<m_strRemoteIp<<":"<<m_dwRemotePort<<"],error:" <<err.message()<<"\n");
        StopConnection();
    }
}
void CSapConnection::ReadPacketCompleted(const void *pBuffer, unsigned int nLen)
{
    SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
    unsigned int dwServiceId=ntohl(pHeader->dwServiceId);
    unsigned int dwMsgId=ntohl(pHeader->dwMsgId);
    unsigned int dwSequence=ntohl(pHeader->dwSequence);
    if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&
        dwServiceId==0&&dwMsgId==0)
    {
        dump_sap_packet_info(pBuffer,E_SAP_IN);
        m_heartResponse.dwSequence=pHeader->dwSequence;
        dump_sap_packet_info(&m_heartResponse,E_SAP_OUT);
        WriteDataInner(&m_heartResponse,sizeof(SSapMsgHeader));
    }
    else if(pHeader->byIdentifer==SAP_PACKET_RESPONSE&&
        dwServiceId==0&&dwMsgId==0)
    {
        dump_sap_packet_info(pBuffer,E_SAP_IN);
    }
    else if(pHeader->byIdentifer==SAP_PACKET_RESPONSE||
        (pHeader->byIdentifer==SAP_PACKET_REQUEST&&pHeader->byM!=0))
    {
        dump_sap_packet_notice(pBuffer,E_SAP_IN);

        if(m_isPeerDigest)
        {
            unsigned char arSignature[16];
            SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
            memcpy(arSignature,pHeader->bySignature,16);

            int nDigestLen=m_strPeerKey.size();
            int nMin=(nDigestLen<16?nDigestLen:16);

            
            memset(pHeader->bySignature,0,16);
            memcpy(pHeader->bySignature,m_strPeerKey.c_str(),nMin);

            CCipher::Md5((const unsigned char *)pBuffer,nLen,pHeader->bySignature);
            if(memcmp(arSignature,pHeader->bySignature,16)!=0)
            {
                ResponseFail(dwServiceId,dwMsgId,dwSequence,SAP_CODE_DEGEST_ERROR);
                return;
            }
        }
        
        if(m_isEnc)
        {
            SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
            unsigned char *ptrLoc=(unsigned char *)pBuffer+pHeader->byHeadLen;
            CCipher::AesDec(m_arEncKey,ptrLoc,nLen-pHeader->byHeadLen,ptrLoc);
        }
        if(m_pOwner!=NULL)
        {
            m_pOwner->OnReadCompleted(pBuffer,nLen);
        }
    }
    else if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&pHeader->byM==0)
    {
        dump_sap_packet_notice(pBuffer,E_SAP_IN);
        ResponseFail(dwServiceId,dwMsgId,dwSequence,SAP_CODE_MAX_FORWARD);
    }
    else
    {
        dump_sap_packet_notice(pBuffer,E_SAP_IN);
    }
}
void CSapConnection::WriteDataInner(const void *pBuffer,int nLen,unsigned int dwTimeout)
{
    SSapLenMsg msg;
    msg.nLen=nLen;
    msg.pBuffer=pBuffer;

    bool write_in_progress = !m_queue.empty();
    m_queue.push_back(msg);
    m_nQueueLen++;
    if (!write_in_progress)//if empty before pushing
    {
        boost::asio::async_write(m_socket,
                  boost::asio::buffer(m_queue.front().pBuffer,m_queue.front().nLen),
                  MakeSapAllocHandler(m_allocWrite,boost::bind(&CSapConnection::HandleWrite, shared_from_this(),
                    boost::asio::placeholders::error)));
    }
}
void CSapConnection::WriteData(const void *pBuffer,int nLen, const void* pExHead, int nExLen,unsigned int dwTimeout)
{
    SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;   
    
    if(m_nQueueLen>MAX_QUEUE_FULL)
    {
        SS_XLOG(XLOG_ERROR,"CSapConnection::%s,queueLen too many[%d], will discard,identifer[%d],target addr[%s:%d]\n",
            __FUNCTION__,m_nQueueLen,pHeader->byIdentifer,m_strRemoteIp.c_str(),m_dwRemotePort);
        if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&m_pOwner!=NULL)
        {
            SSapMsgHeader stSendFailResponse;
            stSendFailResponse.byIdentifer=SAP_PACKET_RESPONSE;
            stSendFailResponse.byHeadLen=sizeof(SSapMsgHeader);
            stSendFailResponse.dwPacketLen=htonl(sizeof(SSapMsgHeader));
            stSendFailResponse.byVersion=0x1;
            stSendFailResponse.dwCode=htonl(SAP_CODE_QUEUE_FULL);

            stSendFailResponse.dwServiceId=pHeader->dwServiceId;
            stSendFailResponse.dwMsgId=pHeader->dwMsgId;
            stSendFailResponse.dwSequence=pHeader->dwSequence;
            m_pOwner->OnReadCompleted(&stSendFailResponse,sizeof(stSendFailResponse));
        }
        return;
    }
    
/*first enc, then digest*/
    int nFactLen=nLen+nExLen;
    unsigned int nLeft=0x10-(nLen-pHeader->byHeadLen)&0x0f;

    if(m_isEnc)
    {
        nFactLen=nLen+nExLen+nLeft;
    }

	void *wrBuffer=malloc(nFactLen);
	memcpy(wrBuffer,pBuffer,pHeader->byHeadLen);
	if (nExLen > 0)
		memcpy((char *)wrBuffer+pHeader->byHeadLen, pExHead, nExLen);
	memcpy((char *)wrBuffer+pHeader->byHeadLen+nExLen, (char *)pBuffer+pHeader->byHeadLen, nLen-pHeader->byHeadLen);
    
    pHeader=(SSapMsgHeader *)wrBuffer;
    pHeader->byHeadLen += nExLen;
    pHeader->dwPacketLen = htonl(ntohl(pHeader->dwPacketLen)+nExLen);

    dump_sap_packet_notice(wrBuffer,E_SAP_OUT);
    if(m_isEnc)
    {
    	if(nLeft>0)
    	{
            memset((char *)wrBuffer+nExLen+nLen,0,nLeft);
    	}
        
        unsigned char *pbyInBlk=(unsigned char *)wrBuffer+pHeader->byHeadLen;
        CCipher::AesEnc(m_arEncKey,pbyInBlk,nFactLen-pHeader->byHeadLen,pbyInBlk);
        pHeader->dwPacketLen=htonl(nFactLen);
    }

    if(m_isLocalDigest)
    {
        int nDigestLen=m_strLocalKey.length();
        int nMin=(nDigestLen<16?nDigestLen:16);
        memcpy(pHeader->bySignature,m_strLocalKey.c_str(),nMin);
        CCipher::Md5((unsigned char *)wrBuffer,nFactLen,pHeader->bySignature);
    }

    WriteDataInner(wrBuffer,nFactLen,dwTimeout);
}

void CSapConnection::WriteData(const void *pBuffer,int nLen,unsigned int dwTimeout)
{
    SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
    dump_sap_packet_notice(pBuffer,E_SAP_OUT);
    
    if(m_nQueueLen>MAX_QUEUE_FULL)
    {
        SS_XLOG(XLOG_ERROR,"CSapConnection::%s,queueLen too many[%d], will discard,identifer[%d],target addr[%s:%d]\n",
            __FUNCTION__,m_nQueueLen,pHeader->byIdentifer,m_strRemoteIp.c_str(),m_dwRemotePort);
        if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&m_pOwner!=NULL)
        {
            SSapMsgHeader stSendFailResponse;
            stSendFailResponse.byIdentifer=SAP_PACKET_RESPONSE;
            stSendFailResponse.byHeadLen=sizeof(SSapMsgHeader);
            stSendFailResponse.dwPacketLen=htonl(sizeof(SSapMsgHeader));
            stSendFailResponse.byVersion=0x1;
            stSendFailResponse.dwCode=htonl(SAP_CODE_QUEUE_FULL);

            stSendFailResponse.dwServiceId=pHeader->dwServiceId;
            stSendFailResponse.dwMsgId=pHeader->dwMsgId;
            stSendFailResponse.dwSequence=pHeader->dwSequence;
            m_pOwner->OnReadCompleted(&stSendFailResponse,sizeof(stSendFailResponse));
        }
        return;
    }
    
/*first enc, then digest*/
    int nFactLen=nLen;
    unsigned int nLeft=0x10-(nLen-pHeader->byHeadLen)&0x0f;

    if(m_isEnc)
    {
        nFactLen=nLen+nLeft;
    }

    void *wrBuffer=malloc(nFactLen);
    memcpy(wrBuffer,pBuffer,nLen);
    pHeader=(SSapMsgHeader *)wrBuffer;
    
    if(m_isEnc)
    {
    	if(nLeft>0)
    	{
            memset((char *)wrBuffer+nLen,0,nLeft);
    	}
        
        unsigned char *pbyInBlk=(unsigned char *)wrBuffer+pHeader->byHeadLen;
        CCipher::AesEnc(m_arEncKey,pbyInBlk,nFactLen-pHeader->byHeadLen,pbyInBlk);
        pHeader->dwPacketLen=htonl(nFactLen);
    }

    if(m_isLocalDigest)
    {
        int nDigestLen=m_strLocalKey.length();
        int nMin=(nDigestLen<16?nDigestLen:16);
        memcpy(pHeader->bySignature,m_strLocalKey.c_str(),nMin);
        CCipher::Md5((unsigned char *)wrBuffer,nFactLen,pHeader->bySignature);
    }

    WriteDataInner(wrBuffer,nFactLen,dwTimeout);
}
void CSapConnection::HandleWrite(const boost::system::error_code& err)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s\n",__FUNCTION__);
    SSapMsgHeader *pHeader=NULL;
    unsigned int dwServiceId=0;
    unsigned int dwMsgId=0;
    if(err)
    {
        SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",addr["<<m_strRemoteIp<<":"<<m_dwRemotePort<<"],error:" <<err.message()<<"\n");
        while(!m_queue.empty())
        {
            SSapMsgHeader *pHeader=(SSapMsgHeader *)(m_queue.front().pBuffer);
            unsigned int dwServiceId=ntohl(pHeader->dwServiceId);
            unsigned int dwMsgId=ntohl(pHeader->dwMsgId);

            if(!(dwServiceId==0&&dwMsgId==0))
            {
                SSapMsgHeader stSendFailResponse;
                stSendFailResponse.byIdentifer=SAP_PACKET_RESPONSE;
                stSendFailResponse.byHeadLen=sizeof(SSapMsgHeader);
                stSendFailResponse.dwPacketLen=htonl(sizeof(SSapMsgHeader));
                stSendFailResponse.byVersion=0x1;
                stSendFailResponse.dwCode=htonl(SAP_CODE_SEND_FAIL);
        
                stSendFailResponse.dwServiceId=pHeader->dwServiceId;
                stSendFailResponse.dwMsgId=pHeader->dwMsgId;
                stSendFailResponse.dwSequence=pHeader->dwSequence;
                if(m_pOwner!=NULL)
                {
                    m_pOwner->OnReadCompleted(&stSendFailResponse,sizeof(stSendFailResponse));
                }
                free((void *)(m_queue.front().pBuffer));
            }
            m_queue.pop_front();
            m_nQueueLen--;
        }
        CloseInner();
        return;
    }
    
    pHeader = (SSapMsgHeader *)(m_queue.front().pBuffer);
    dwServiceId = ntohl(pHeader->dwServiceId);
    dwMsgId = ntohl(pHeader->dwMsgId);
    if(!(dwServiceId==0&&dwMsgId==0))
    {
        free((void *)(m_queue.front().pBuffer));
    }
    
    m_queue.pop_front();
    m_nQueueLen--;
    if (!m_queue.empty())
    {
        boost::asio::async_write(m_socket,
              boost::asio::buffer(m_queue.front().pBuffer,m_queue.front().nLen),
              MakeSapAllocHandler(m_allocWrite,boost::bind(&CSapConnection::HandleWrite, shared_from_this(),
                boost::asio::placeholders::error)));
    }
}

void CSapConnection::OnTimer()
{   
    if(bClientType==true)
    {
        m_heartRequest.dwSequence=htonl(m_heartSequence++);
        dump_sap_packet_info(&m_heartRequest,E_SAP_OUT);
        WriteDataInner(&m_heartRequest,sizeof(SSapMsgHeader));
    }

    struct timeval_a now,interval,endtime;
    gettimeofday_a(&now,NULL);
    interval.tv_sec=sm_nTimeoutInterval;
    interval.tv_usec=0;
    timeradd(&m_AlivePoint,&interval,&endtime);
    if(timercmp(&now,&endtime,>))
    {
        SS_XLOG(XLOG_WARNING,"CSapConnection::%s, will close[%s:%d].alive[%ld.%06ld] now[%ld.%06ld]\n",
            __FUNCTION__,m_strRemoteIp.c_str(),m_dwRemotePort, m_AlivePoint.tv_sec,m_AlivePoint.tv_usec,
            now.tv_sec,now.tv_usec);
        CloseInner();
    }
}
void CSapConnection::Close()
{
    bRunning=false;
    CloseInner();
}
void CSapConnection::CloseInner()
{
    SS_XLOG(XLOG_TRACE,"CSapConnection::%s\n",__FUNCTION__);     
        
    boost::system::error_code ignore_ec;
    m_socket.close(ignore_ec);
    m_tmHeartBeat.Stop();
    m_tmHeartBeat.Init();
}

void CSapConnection::StopConnection()
{
    boost::system::error_code ignore_ec;
    m_socket.close(ignore_ec);
    m_tmHeartBeat.Stop();
    m_tmHeartBeat.Init();
    if(bRunning)
    {
        bRunning=false;
        if(m_pOwner!=NULL)
        {
            m_pOwner->OnPeerClose();
        }
    }
}
void CSapConnection::SetLocalDigest(const string &strDigest)
{
    m_isLocalDigest=true;
	m_strLocalKey=strDigest;
}
void CSapConnection::SetPeerDigest(const string &strDigest)
{
    m_isPeerDigest=true;
    m_strPeerKey=strDigest;
}
void CSapConnection::SetEncKey(unsigned char arKey[16])
{
    m_isEnc=true;
	memcpy(m_arEncKey,arKey,16);
}
void CSapConnection::ResponseFail(unsigned int dwServiceId,unsigned int dwMsgId,unsigned int dwSequence,unsigned int dwCode)
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
    WriteData(&response,sizeof(SSapMsgHeader));
}

void CSapConnection::dump_sap_packet_notice(const void *pBuffer,ESapDirectType type)
{
    if(IsEnableLevel(SAP_STACK_MODULE,XLOG_NOTICE))
    {
        string strPacket;
        char szPeer[128]={0};
        sprintf(szPeer,"%s:%d",m_strRemoteIp.c_str() ,m_dwRemotePort);

        if(type==E_SAP_IN)
        {
            strPacket=string("Read Sap Command from  addr[")+szPeer+"]\n";
        }
        else
        {
            strPacket=string("Write Sap Command to  addr[")+szPeer+"]\n";
        }
        
        SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
        
        unsigned char *pChar=(unsigned char *)pBuffer;
        unsigned int nPacketlen=ntohl(pHeader->dwPacketLen);
        int nLine=nPacketlen>>3;
        int nLast=(nPacketlen&0x7);
        int i=0;
        for(i=0;(i<nLine)&&(i<200);i++)
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
        SS_XLOG(XLOG_NOTICE,strPacket.c_str());
    }
}
void CSapConnection::dump_sap_packet_info(const void *pBuffer,ESapDirectType type)
{
    if(IsEnableLevel(SAP_STACK_MODULE,XLOG_INFO))
    {
        string strPacket;
        char szPeer[64]={0};
        sprintf(szPeer,"%s:%d",m_strRemoteIp.c_str() ,m_dwRemotePort);

        if(type==E_SAP_IN)
        {
            strPacket=string("Read Sap Command from addr[")+szPeer+"]\n";
        }
        else
        {
            strPacket=string("Write Sap Command to addr[")+szPeer+"]\n";
        }
        
        SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
        
        unsigned char *pChar=(unsigned char *)pBuffer;
        unsigned int nPacketlen=ntohl(pHeader->dwPacketLen);
        int nLine=nPacketlen>>3;
        int nLast=(nPacketlen&0x7);
        int i=0;
        for(i=0;(i<nLine) && (i<200);i++)
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
        SS_XLOG(XLOG_INFO,strPacket.c_str());
    }
}
void CSapConnection::Dump()
{
    SS_XLOG(XLOG_NOTICE,"@@@@      manager[%x],own[%x],addr[%s:%d],type[%d]\n",
        m_pManager,m_pOwner,m_strRemoteIp.c_str(),m_dwRemotePort,bClientType);
    SS_XLOG(XLOG_NOTICE,"           isLocalKey[%d],localkey[%s]\n",m_isLocalDigest,m_strLocalKey.c_str());
    SS_XLOG(XLOG_NOTICE,"           isPeerDigest[%d],PeerKey[%s]\n",m_isPeerDigest,m_strPeerKey.c_str());
    SS_XLOG(XLOG_NOTICE,"           isEnc[%d],enckey[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
        m_isEnc,m_arEncKey[0],m_arEncKey[1],m_arEncKey[2],m_arEncKey[3],
        m_arEncKey[4],m_arEncKey[5],m_arEncKey[6],m_arEncKey[7],
        m_arEncKey[8],m_arEncKey[9],m_arEncKey[10],m_arEncKey[11],
        m_arEncKey[12],m_arEncKey[13],m_arEncKey[14],m_arEncKey[15]);
}


