#include "SapConnection.h"
#include "MemoryPool.h"
#include <boost/bind.hpp>
#include <iostream>
#include "SapLogHelper.h"
#include "SapMessage.h"
#include "Cipher.h"
#include "CommonMacro.h"

#define MAX_SAP_PAKET_LEN 1024*1024*2
const int HeartInterval = 50;
const int TimeoutInterval = 120;


boost::mutex CSapConnection::sm_mutCount;
unsigned int CSapConnection::sm_nMark = 0;


CSapConnection::CSapConnection(boost::asio::io_service &io_service, CTimerManager *pManager,ISapSession *pSession):
    m_socket(io_service),m_resolver(io_service),
	m_isConnected(false),bRunning(true),m_nQueueLen(0), m_strRemoteIp(""),m_dwRemotePort(0),
	m_isLocalDigest(false),m_isPeerDigest(false),m_isEnc(false), m_pTimerManager(pManager),m_pSession(pSession),	
	m_tmHeartBeat((CTimerManager *)pManager)
{
	sm_mutCount.lock();
	m_nId = sm_nMark++;
	sm_mutCount.unlock();
	
    m_heartSequence=0;
    m_heartRequest.SetService(SAP_PACKET_REQUEST,0,0,0);
    m_heartRequest.SetSequence(m_heartSequence++);

    gettimeofday_a(&m_HeartBeatPoint,NULL);
    gettimeofday_a(&m_AlivePoint,NULL);
}

CSapConnection::~CSapConnection()
{
	SS_XLOG(XLOG_TRACE,"CSapConnection::%s\n", __FUNCTION__);	
}

void CSapConnection::AsyncConnect(const string &strIp,unsigned int dwPort,int nTimeout,unsigned int dwLocalPort)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::async_connect,id[%d],addr[%s:%d],timeout[%d]\n",m_nId,strIp.c_str(),dwPort,nTimeout);
    m_isConnected=false;
    
    char szBuf[16]={0};
    sprintf(szBuf,"%d",dwPort);
    tcp::resolver::query query(strIp,szBuf);
    m_resolver.async_resolve(query,
        MakeSapAllocHandler(m_allocReader, boost::bind(&CSapConnection::HandleResolve, shared_from_this(),
              boost::asio::placeholders::error, boost::asio::placeholders::iterator,dwLocalPort)));
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
                SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],open socket error:" <<ec.message()<<"\n");
                ConnectFinish(-1);
                return;
            }
			m_socket.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true),ec);
			m_socket.set_option(boost::asio::ip::tcp::no_delay(true),ec);
            m_socket.bind(tcp::endpoint(tcp::v4(),dwLocalPort),ec);
            if(ec)
            {
                SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],bind port error:" <<ec.message()<<"\n");
                ConnectFinish(-1);
                return;
            }
        }
        tcp::endpoint endpoint = *endpoint_iterator;
        SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d],addr[%s:%d]\n",__FUNCTION__,m_nId,endpoint.address().to_string() .c_str() ,endpoint.port());
        m_socket.async_connect(endpoint,
            MakeSapAllocHandler(m_allocReader, boost::bind(&CSapConnection::HandleConnected, shared_from_this(),
            	boost::asio::placeholders::error, ++endpoint_iterator)));
    }
    else
    {
      SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],error:" <<err.message()<<"\n");
      ConnectFinish(-2);
    }
}


void CSapConnection::HandleConnected(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);
    if (!err)
    {
        gettimeofday_a(&m_HeartBeatPoint,NULL);
        gettimeofday_a(&m_AlivePoint,NULL);
        ConnectFinish(0);
        m_isConnected=true;
        SetRemoteAddr();
		
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
        //SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],addr[], error:" <<err.message()<<"\n");
		SS_XLOG(XLOG_WARNING,"CSapConnection::%s, id[%d],  error:%s\n",	__FUNCTION__,m_nId,(err.message()).c_str());
        ConnectFinish(-3);
    }
}

void CSapConnection::ConnectFinish(int result)
{
	SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d],result[%d]\n",__FUNCTION__,m_nId,result);
	if(m_pSession != NULL)
	    m_pSession->OnConnectResult(m_nId,result);
    
}

void CSapConnection::AsyncRead()
{
    m_socket.get_io_service().post(boost::bind(&CSapConnection::AsyncReadInner, shared_from_this()));
}

void CSapConnection::AsyncReadInner()
{
    gettimeofday_a(&m_HeartBeatPoint,NULL);
    gettimeofday_a(&m_AlivePoint,NULL);
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);

	m_tmHeartBeat.Init((CTimerManager *)m_pTimerManager,60,boost::bind(&CSapConnection::DoDetectInterval,shared_from_this()),CThreadTimer::TIMER_CIRCLE);
	m_tmHeartBeat.Start();

    m_pHeader=buffer_.base();
    m_socket.async_read_some( boost::asio::buffer(buffer_.top(),buffer_.capacity()),
					MakeSapAllocHandler(m_allocReader,boost::bind(&CSapConnection::HandleRead, shared_from_this(),
							boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred)));
}

void CSapConnection::HandleRead(const boost::system::error_code& err,std::size_t dwTransferred)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d],bytes_transferred[%d]\n",__FUNCTION__,m_nId,dwTransferred);
    if(!err)
    {
        gettimeofday_a(&m_AlivePoint,NULL);
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
                SS_XLOG(XLOG_WARNING,"CSapConnection::%s,id[%d],packetlen[%d],identifier[%d], will close this socket\n",__FUNCTION__,m_nId,packetlen,ptrHeader->byIdentifer);
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
        SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],error:" <<err.message()<<"\n");
        StopConnection();
    }
}


void CSapConnection::ReadPacketCompleted(const void *pBuffer, unsigned int nLen)
{
    SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
    unsigned int dwServiceId=ntohl(pHeader->dwServiceId);
    unsigned int dwMsgId=ntohl(pHeader->dwMsgId);
   
    if(pHeader->byIdentifer==SAP_PACKET_RESPONSE&&
        dwServiceId==0&&dwMsgId==0) //heart-beat packet
    {
        dump_sap_packet_info(pBuffer,E_SAP_IN);
    }
    else if(pHeader->byIdentifer==SAP_PACKET_RESPONSE)
    {       
        ReadAvenueResponsePacketCompleted(pBuffer,nLen);	
    }
	else if(pHeader->byIdentifer==SAP_PACKET_REQUEST)
	{
		ReadAvenueRequestPacketCompleted(pBuffer,nLen);
	}
    else
    {
        dump_sap_packet_notice(pBuffer,E_SAP_IN);
    }
}

void CSapConnection::ReadAvenueResponsePacketCompleted(const void *pBuffer, unsigned int nLen)
{
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
            return;
        }
    }
    
    if(m_isEnc)
    {
        SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
        unsigned char *ptrLoc=(unsigned char *)pBuffer+pHeader->byHeadLen;
        CCipher::AesDec(m_arEncKey,ptrLoc,nLen-pHeader->byHeadLen,ptrLoc);
    }
    dump_sap_packet_notice(pBuffer,E_SAP_IN);
	if(m_pSession != NULL)
	    m_pSession->OnReceiveSosAvenueResponse(m_nId,pBuffer,nLen,m_strRemoteIp,m_dwRemotePort);
}

void CSapConnection::ReadAvenueRequestPacketCompleted(const void *pBuffer, unsigned int nLen)
{
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
            //ResponseFail(8,8,ntohl(pHeader->dwSequence),SAP_CODE_DEGEST_ERROR);
            return;
        }
    }
    
    if(m_isEnc)
    {
        SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
        unsigned char *ptrLoc=(unsigned char *)pBuffer+pHeader->byHeadLen;
        CCipher::AesDec(m_arEncKey,ptrLoc,nLen-pHeader->byHeadLen,ptrLoc);
    }
    dump_sap_packet_notice(pBuffer,E_SAP_IN);
	if(m_pSession != NULL)
	    m_pSession->OnReceiveAvenueRequest(m_nId,pBuffer,nLen,m_strRemoteIp,m_dwRemotePort);
}


void CSapConnection::WriteData(const void *pBuffer,int nLen,const char* pSignatureKey,unsigned int dwTimeout)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);
    dump_sap_packet_notice(pBuffer,E_SAP_OUT);
    SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;

	if(m_nQueueLen>MAX_QUEUE_FULL)
     {
        SS_XLOG(XLOG_ERROR,"CSapConnection::%s,queueLen too many[%d], will discard,identifer[%d],target addr[%s:%d]\n",
            __FUNCTION__,m_nQueueLen,pHeader->byIdentifer,m_strRemoteIp.c_str(),m_dwRemotePort);
        if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&m_pSession!=NULL)
        {
            SSapMsgHeader stSendFailResponse;
            stSendFailResponse.byIdentifer=SAP_PACKET_RESPONSE;
            stSendFailResponse.byHeadLen=sizeof(SSapMsgHeader);
            stSendFailResponse.dwPacketLen=htonl(sizeof(SSapMsgHeader));
            stSendFailResponse.byVersion=0x1;
            stSendFailResponse.dwCode=htonl(ERROR_SOS_QUEUE_FULL);

            stSendFailResponse.dwServiceId=pHeader->dwServiceId;
            stSendFailResponse.dwMsgId=pHeader->dwMsgId;
            stSendFailResponse.dwSequence=pHeader->dwSequence;
			m_pSession->OnReceiveSosAvenueResponse(m_nId,&stSendFailResponse,sizeof(stSendFailResponse),m_strRemoteIp,m_dwRemotePort);
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
            memset((char *)wrBuffer+nLen,0,nLeft);
        
        unsigned char *pbyInBlk=(unsigned char *)wrBuffer+pHeader->byHeadLen;
        CCipher::AesEnc(m_arEncKey,pbyInBlk,nFactLen-pHeader->byHeadLen,pbyInBlk);
        pHeader->dwPacketLen=htonl(nFactLen);
    }

    SS_XLOG(XLOG_TRACE,"CSapConnection::%s,id[%d],m_isLocalDigest[%d],m_isEnc[%d],key[%s]\n",__FUNCTION__,m_nId,m_isLocalDigest,m_isEnc,m_strLocalKey.c_str());
    if(pSignatureKey != NULL && strlen(pSignatureKey) > 0)
    {
        int nDigestLen=strlen(pSignatureKey);
        int nMin=(nDigestLen<16?nDigestLen:16);
        memcpy(pHeader->bySignature,pSignatureKey,nMin);
        CCipher::Md5((unsigned char *)wrBuffer,nFactLen,pHeader->bySignature);
    }

    WriteDataInner(wrBuffer,nFactLen,dwTimeout);
}

void CSapConnection::WriteDataEx(const void *pBuffer,int nLen, const void* pExHead, int nExLen,const char* pSignatureKey,unsigned int dwTimeout)
{
     SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d], nLen[%d],nExLen[%d]\n",__FUNCTION__,m_nId,nLen,nExLen);
	 
	 SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;

	 if(m_nQueueLen>MAX_QUEUE_FULL)
     {
        SS_XLOG(XLOG_ERROR,"CSapConnection::%s,queueLen too many[%d], will discard,identifer[%d],target addr[%s:%d]\n",
            __FUNCTION__,m_nQueueLen,pHeader->byIdentifer,m_strRemoteIp.c_str(),m_dwRemotePort);
        if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&m_pSession!=NULL)
        {
            SSapMsgHeader stSendFailResponse;
            stSendFailResponse.byIdentifer=SAP_PACKET_RESPONSE;
            stSendFailResponse.byHeadLen=sizeof(SSapMsgHeader);
            stSendFailResponse.dwPacketLen=htonl(sizeof(SSapMsgHeader));
            stSendFailResponse.byVersion=0x1;
            stSendFailResponse.dwCode=htonl(ERROR_SOS_QUEUE_FULL);

            stSendFailResponse.dwServiceId=pHeader->dwServiceId;
            stSendFailResponse.dwMsgId=pHeader->dwMsgId;
            stSendFailResponse.dwSequence=pHeader->dwSequence;
            m_pSession->OnReceiveSosAvenueResponse(m_nId,&stSendFailResponse,sizeof(stSendFailResponse),m_strRemoteIp,m_dwRemotePort);
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

	 SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d], nLen[%d],nExLen[%d],nFactLen[%d]\n",__FUNCTION__,m_nId,nLen,nExLen,nFactLen);

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

	if(pSignatureKey != NULL && strlen(pSignatureKey) > 0)
	{
		int nDigestLen=strlen(pSignatureKey);
		int nMin=(nDigestLen<16?nDigestLen:16);
		memcpy(pHeader->bySignature,pSignatureKey,nMin);
		CCipher::Md5((unsigned char *)wrBuffer,nFactLen,pHeader->bySignature);
	}

    WriteDataInner(wrBuffer,nFactLen,dwTimeout);
}


void CSapConnection::WriteDataInner(const void *pBuffer,int nLen,unsigned int dwTimeout)
{
    SSapLenMsg msg;
    msg.nLen=nLen;
    msg.pBuffer=pBuffer;

    bool write_in_progress = !m_queue.empty();
    m_queue.push_back(msg);
	m_nQueueLen++;
    if (!write_in_progress)
    {
        boost::asio::async_write(m_socket, boost::asio::buffer(m_queue.front().pBuffer,m_queue.front().nLen),
                  MakeSapAllocHandler(m_allocWrite,boost::bind(&CSapConnection::HandleWrite, shared_from_this(),
                    boost::asio::placeholders::error)));
    }
}

void CSapConnection::HandleWrite(const boost::system::error_code& err)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);
    SSapMsgHeader *pHeader=NULL;
    unsigned int dwServiceId=0;
    unsigned int dwMsgId=0;
    if(err)
    {
        SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],error:" <<err.message()<<"\n");
        while(!m_queue.empty())
        {
            SSapMsgHeader *pHeader=(SSapMsgHeader *)(m_queue.front().pBuffer);
            unsigned int dwServiceId=ntohl(pHeader->dwServiceId);
            unsigned int dwMsgId=ntohl(pHeader->dwMsgId);
            if(!(dwServiceId==0&&dwMsgId==0))
            {
                if(pHeader->byIdentifer==SAP_PACKET_REQUEST && m_pSession!=NULL)
                {
                    SSapMsgHeader stSendFailResponse;
                    stSendFailResponse.byIdentifer=SAP_PACKET_RESPONSE;
                    stSendFailResponse.byHeadLen=sizeof(SSapMsgHeader);
                    stSendFailResponse.dwPacketLen=htonl(sizeof(SSapMsgHeader));
                    stSendFailResponse.byVersion=0x1;
                    stSendFailResponse.dwCode=htonl(ERROR_SOS_SEND_FAIL);
            
                    stSendFailResponse.dwServiceId=pHeader->dwServiceId;
                    stSendFailResponse.dwMsgId=pHeader->dwMsgId;
                    stSendFailResponse.dwSequence=pHeader->dwSequence;
                    m_pSession->OnReceiveSosAvenueResponse(m_nId,&stSendFailResponse,sizeof(stSendFailResponse),m_strRemoteIp,m_dwRemotePort);
                }
                free((void *)(m_queue.front().pBuffer));
            }
            m_queue.pop_front();
			m_nQueueLen--;
        }
        HandleStop();
        return;
    }
    
    pHeader=(SSapMsgHeader *)(m_queue.front().pBuffer);
    dwServiceId=ntohl(pHeader->dwServiceId);
    dwMsgId=ntohl(pHeader->dwMsgId);
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

void CSapConnection::DoDetectInterval()
{
    struct timeval_a now;
    gettimeofday_a(&now,NULL);
  
    struct timeval_a interval,endtime;
      interval.tv_sec=HeartInterval;
    interval.tv_usec=0;
    timeradd(&m_HeartBeatPoint,&interval,&endtime);
    if(timercmp(&now,&endtime,>=))
	{
        m_heartRequest.SetSequence(m_heartSequence++);
        dump_sap_packet_info(m_heartRequest.GetBuffer(),E_SAP_OUT);
        WriteDataInner(m_heartRequest.GetBuffer(),m_heartRequest.GetLen());
        m_HeartBeatPoint=now;
    }

    interval.tv_sec=TimeoutInterval;
    interval.tv_usec=0;
    timeradd(&m_AlivePoint,&interval,&endtime);
    if(timercmp(&now,&endtime,>))
    {
        SS_XLOG(XLOG_WARNING,"CSapConnection::%s,id[%d] timeout, will reconnect.alive[%ld.%06ld] now[%ld.%06ld]\n",
            __FUNCTION__,m_nId,m_AlivePoint.tv_sec,m_AlivePoint.tv_usec,
            now.tv_sec,now.tv_usec);
        StopConnection();
    }
}


void CSapConnection::Close()
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);
    bRunning=false;
    HandleStop();    
}

void CSapConnection::HandleStop()
{
    SS_XLOG(XLOG_TRACE,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);
    boost::system::error_code ignore_ec;
    m_socket.close(ignore_ec);
}

void CSapConnection::StopConnection()
{
	SS_XLOG(XLOG_TRACE,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);
    boost::system::error_code ignore_ec;
    m_socket.close(ignore_ec);
    bRunning=false;
	m_isConnected = false;
	if(m_pSession != NULL)
	    m_pSession->OnPeerClose(m_nId);
	
	m_tmHeartBeat.Stop();

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
        return;
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

void CSapConnection::dump_sap_packet_notice(const void *pBuffer,ESapDirectType type)
{
    if(IsEnableLevel(SAP_STACK_MODULE,XLOG_NOTICE))
    {
        string strPacket;
        char szPeer[128]={0};
        char szId[16]={0};
        sprintf(szId,"%d",m_nId);
        sprintf(szPeer,"%s:%d",m_strRemoteIp.c_str() ,m_dwRemotePort);

        if(type==E_SAP_IN)
        {
            strPacket=string("Read Sap Command from id[")+szId+"] addr["+szPeer+"]\n";
        }
        else
        {
            strPacket=string("Write Sap Command to id[")+szId+"] addr["+szPeer+"]\n";
        }
        
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
            snprintf(szBuf,127,"                [%2d]    %02x %02x %02x %02x    %02x %02x %02x %02x    ......\n",
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
        char szId[16]={0};
        sprintf(szId,"%d",m_nId);
        sprintf(szPeer,"%s:%d",m_strRemoteIp.c_str() ,m_dwRemotePort);

        if(type==E_SAP_IN)
        {
            strPacket=string("Read Sap Command from id[")+szId+"] addr["+szPeer+"]\n";
        }
        else
        {
            strPacket=string("Write Sap Command to id[")+szId+"] addr["+szPeer+"]\n";
        }
        
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
            snprintf(szBuf,127,"                [%2d]    %02x %02x %02x %02x    %02x %02x %02x %02x    ......\n",
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

