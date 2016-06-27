#include "SapConnection.h"
#include "MemoryPool.h"
#include <boost/bind.hpp>
#include <iostream>
#include "SapLogHelper.h"
#include "SapMessage.h"
#include "SapConnManager.h"
#include "Cipher.h"
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
#define MAX_SAP_PAKET_LEN 1024*1024*2

using sdo::common::CMemoryPool;
using namespace sdo::common;
namespace sdo{
namespace sap{
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

    
int CSapConnection::sm_nHeartInterval=50;
int CSapConnection::sm_nTimeoutInterval=120;


CSapConnection::CSapConnection(CSapAgent *pAgent,boost::asio::io_service &io_service):
    m_pAgent(pAgent),m_socket(io_service),m_resolver(io_service),m_timer(io_service),
    bClientType(false),bRunning(true),m_isConnected(true),
    m_isLocalDigest(false),m_isPeerDigest(false),m_isEnc(false)
{
    m_heartSequence=0;
    m_heartRequest.SetService(SAP_PACKET_REQUEST,0,0,0);
    m_heartRequest.SetSequence(m_heartSequence++);

    m_heartResponse.SetService(SAP_PACKET_RESPONSE,0,0,0);
    m_heartResponse.SetSequence(0);

    gettimeofday_a(&m_HeartBeatPoint,NULL);
    gettimeofday_a(&m_AlivePoint,NULL);
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
CSapConnection::~CSapConnection()
{
    SS_XLOG(XLOG_TRACE,"CSapConnection::~CSapConnection,id[%d]\n",m_nId);
    m_mapReceive.clear();
}
void CSapConnection::AsyncConnect(const string &strIp,unsigned int dwPort,int nTimeout,unsigned int dwLocalPort)
{
    SS_XLOG(XLOG_TRACE,"CSapConnection::async_connect,id[%d],addr[%s:%d],timeout[%d]\n",m_nId,strIp.c_str(),dwPort,nTimeout);
    bClientType=true;
    m_isConnected=false;


#ifndef _NO_USE_RESOLVE_ADDR
    char szBuf[16]={0};
    sprintf(szBuf,"%d",dwPort);
    tcp::resolver::query query(strIp,szBuf);
    m_resolver.async_resolve(query,
        MakeSapAllocHandler(m_allocReader,
        boost::bind(&CSapConnection::HandleResolve, shared_from_this(),
              boost::asio::placeholders::error,
              boost::asio::placeholders::iterator,dwLocalPort)));
#else
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
	boost::system::error_code  err;
	boost::asio::ip::address_v4 addrV4=boost::asio::ip::address_v4::from_string(strIp,err);
	if(err)
	{
		CS_XLOG(XLOG_WARNING,"CSapConnection::%s,connection[%0x],invalid addr[%s:%d]\n",__FUNCTION__,this,strIp.c_str(),dwPort);
		return;
	}
	tcp::endpoint endpoint(addrV4,dwPort);
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d],addr[%s:%d]\n",__FUNCTION__,m_nId,endpoint.address().to_string().c_str() ,endpoint.port());
    m_socket.async_connect(endpoint,
        MakeSapAllocHandler(m_allocReader,
      boost::bind(&CSapConnection::HandleConnected, shared_from_this(),
        boost::asio::placeholders::error)));

#endif
        
    
    m_timer.expires_from_now(boost::posix_time::seconds(nTimeout));
	m_timer.async_wait(boost::bind(&CSapConnection::HandleConnectTimeout, 
        shared_from_this(),boost::asio::placeholders::error));
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
            MakeSapAllocHandler(m_allocReader,
          boost::bind(&CSapConnection::HandleConnected, shared_from_this(),
            boost::asio::placeholders::error, ++endpoint_iterator)));
    }
    else
    {
      SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],error:" <<err.message()<<"\n");
      ConnectFinish(-2);
    }
}

void CSapConnection::HandleConnected(const boost::system::error_code& err)
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
	else
	{
		SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],error:" <<err.message()<<"\n");
		ConnectFinish(-3);
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
        SS_SLOG(XLOG_WARNING,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],error:" <<err.message()<<"\n");
        ConnectFinish(-3);
    }
}
void CSapConnection::ConnectFinish(int result)
{
    boost::system::error_code ignore_ec;
    m_timer.cancel(ignore_ec);
    m_pAgent->OnConnectResult(m_nId,result);
    if(result!=0)
    {
        HandleStop();
    }
}
void CSapConnection::HandleConnectTimeout(const boost::system::error_code& err)
{
    SS_SLOG(XLOG_DEBUG,"CSapConnection::"<<__FUNCTION__<<",id["<<m_nId<<"],code:" <<err.message()<<"\n");
    if(m_isConnected==false)
    {
        boost::system::error_code ignore_ec;
        m_socket.close(ignore_ec);
    }
}

void CSapConnection::AsyncRead()
{
    gettimeofday_a(&m_HeartBeatPoint,NULL);
    gettimeofday_a(&m_AlivePoint,NULL);
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);

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
                //ResponseFail(8,8,ntohl(ptrHeader->dwSequence),SAP_CODE_PACKET_ERROR);
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
    unsigned int dwSequence=ntohl(pHeader->dwSequence);
    if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&
        dwServiceId==0&&dwMsgId==0)
    {
        dump_sap_packet_info(pBuffer,E_SAP_IN);
        m_heartResponse.SetSequence(dwSequence);
        dump_sap_packet_info(m_heartResponse.GetBuffer(),E_SAP_OUT);
        WriteData(m_heartResponse.GetBuffer(),m_heartResponse.GetLen());
    }
    else if(pHeader->byIdentifer==SAP_PACKET_RESPONSE&&
        dwServiceId==0&&dwMsgId==0)
    {
        dump_sap_packet_info(pBuffer,E_SAP_IN);
    }
    else if(pHeader->byIdentifer==SAP_PACKET_RESPONSE)
    {
        //dump_sap_packet_notice(pBuffer,E_SAP_IN);
        unsigned int dwCode=ntohl(pHeader->dwCode);
        if(dwCode>=100 && dwCode<200)
        {
            if(m_oRequestList.IsExistSequence(pHeader->dwSequence))
                ReadBusinessPacketCompleted(pBuffer,nLen);
        }
        else
        {
            if(m_oRequestList.RemoveHistoryBySequence(pHeader->dwSequence,dwCode))
                ReadBusinessPacketCompleted(pBuffer,nLen);
        }
    }
    else if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&pHeader->byM!=0)
    {
        //dump_sap_packet_notice(pBuffer,E_SAP_IN);
        if(IsEnableLevel(SAPPER_STACK_MODULE,XLOG_INFO))
        {
            AddReceiveRequestHistory(pBuffer);
        }
        ReadBusinessPacketCompleted(pBuffer,nLen);
    }
    else if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&pHeader->byM==0)
    {
        dump_sap_packet_notice(pBuffer,E_SAP_IN);
        pHeader->byIdentifer=SAP_PACKET_RESPONSE;
        pHeader->dwCode=htonl(SAP_CODE_MAX_FORWARD);
        if(IsEnableLevel(SAPPER_STACK_MODULE,XLOG_INFO))
        {
            AddReceiveRequestHistory(pBuffer);
        }
        AsyncWrite(pBuffer,sizeof(SSapMsgHeader));
    }
    else
    {
        dump_sap_packet_notice(pBuffer,E_SAP_IN);
    }
}
void CSapConnection::ReadBusinessPacketCompleted(const void *pBuffer, unsigned int nLen)
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
            ResponseFail(8,8,ntohl(pHeader->dwSequence),SAP_CODE_DEGEST_ERROR);
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
    m_pAgent->OnReceiveMessage(m_nId,pBuffer,nLen,m_strRemoteIp,m_dwRemotePort);
}
void CSapConnection::WriteData(const void *pBuffer,int nLen,unsigned int dwTimeout)
{
    SSapLenMsg msg;
    msg.nLen=nLen;
    msg.pBuffer=pBuffer;

    SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
    unsigned int dwServiceId=pHeader->dwServiceId;
    unsigned int dwMsgId=pHeader->dwMsgId;
    if((!(dwServiceId==0&&dwMsgId==0))&&pHeader->byIdentifer==SAP_PACKET_REQUEST)
    {
        m_oRequestList.AddHistory(m_nId,pHeader->dwSequence,dwServiceId,dwMsgId,dwTimeout);
    }
    else if((!(dwServiceId==0&&dwMsgId==0))&&pHeader->byIdentifer==SAP_PACKET_RESPONSE)
    {
        if(IsEnableLevel(SAPPER_STACK_MODULE,XLOG_INFO))
        {
            RemoveReceiveRequestHistory(pBuffer);
        }
    }
    bool write_in_progress = !m_queue.empty();
    m_queue.push_back(msg);
    if (!write_in_progress)
    {
        boost::asio::async_write(m_socket,
                  boost::asio::buffer(m_queue.front().pBuffer,m_queue.front().nLen),
                  MakeSapAllocHandler(m_allocWrite,boost::bind(&CSapConnection::HandleWrite, shared_from_this(),
                    boost::asio::placeholders::error)));
    }
}
void CSapConnection::AsyncWrite(const void *pBuffer,int nLen,unsigned int dwTimeout)
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);
    dump_sap_packet_notice(pBuffer,E_SAP_OUT);

    /*first enc, then digest*/
    int nFactLen=nLen;
    SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
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
    if(m_isLocalDigest)
    {
        int nDigestLen=m_strLocalKey.length();
        int nMin=(nDigestLen<16?nDigestLen:16);
        memcpy(pHeader->bySignature,m_strLocalKey.c_str(),nMin);
        CCipher::Md5((unsigned char *)wrBuffer,nFactLen,pHeader->bySignature);
    }

    //WriteData(wrBuffer,nLen);
    m_socket.get_io_service().post(boost::bind(&CSapConnection::WriteData, shared_from_this(),wrBuffer,nFactLen,dwTimeout));
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
                if(pHeader->byIdentifer==SAP_PACKET_REQUEST&&
                    m_oRequestList.RemoveHistoryBySequence(pHeader->dwSequence,SAP_CODE_SEND_FAIL))
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
                    m_pAgent->OnReceiveMessage(m_nId,&stSendFailResponse,sizeof(stSendFailResponse),m_strRemoteIp,m_dwRemotePort);
                }
                free((void *)(m_queue.front().pBuffer));
            }
            m_queue.pop_front();
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
    if (!m_queue.empty())
    {
        boost::asio::async_write(m_socket,
              boost::asio::buffer(m_queue.front().pBuffer,m_queue.front().nLen),
              MakeSapAllocHandler(m_allocWrite,boost::bind(&CSapConnection::HandleWrite, shared_from_this(),
                boost::asio::placeholders::error)));
    }
}
void CSapConnection::DetectInterval()
{
    m_socket.get_io_service().post(boost::bind(&CSapConnection::DoDetectInterval, shared_from_this()));
}

void CSapConnection::DoDetectInterval()
{
    m_oRequestList.DetectTimerList();

    
    struct timeval_a now;
    gettimeofday_a(&now,NULL);
    
    struct timeval_a interval,endtime;
    interval.tv_sec=sm_nHeartInterval;
    interval.tv_usec=0;
    timeradd(&m_HeartBeatPoint,&interval,&endtime);
    if(bClientType==true&&timercmp(&now,&endtime,>=))
    {
        m_heartRequest.SetSequence(m_heartSequence++);
        dump_sap_packet_info(m_heartRequest.GetBuffer(),E_SAP_OUT);
        WriteData(m_heartRequest.GetBuffer(),m_heartRequest.GetLen());
        m_HeartBeatPoint=now;
    }

    interval.tv_sec=sm_nTimeoutInterval;
    interval.tv_usec=0;
    timeradd(&m_AlivePoint,&interval,&endtime);
    if(timercmp(&now,&endtime,>))
    {
        SS_XLOG(XLOG_WARNING,"CSapConnection::%s,id[%d] timeout, will close.alive[%ld.%06ld] now[%ld.%06ld]\n",
            __FUNCTION__,m_nId,m_AlivePoint.tv_sec,m_AlivePoint.tv_usec,
            now.tv_sec,now.tv_usec);
        HandleStop();
    }
}
void CSapConnection::Close()
{
    SS_XLOG(XLOG_DEBUG,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);
    bRunning=false;
    m_socket.get_io_service().post(boost::bind(&CSapConnection::HandleStop,shared_from_this()));
    
}
void CSapConnection::HandleStop()
{
    SS_XLOG(XLOG_TRACE,"CSapConnection::%s,id[%d]\n",__FUNCTION__,m_nId);
    boost::system::error_code ignore_ec;
    m_socket.close(ignore_ec);
}
void CSapConnection::StopConnection()
{
    boost::system::error_code ignore_ec;
    m_socket.close(ignore_ec);
    if(bRunning)
    {
        bRunning=false;
        m_pAgent->OnPeerClose(m_nId);

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

void CSapConnection::AddReceiveRequestHistory(const void * pBuffer)
{
    SSapMsgHeader *pHeader=(SSapMsgHeader *)(pBuffer);
    SSapRequestHistory oRequest;
    oRequest.nId=m_nId;
    oRequest.dwSequence=htonl(pHeader->dwSequence);
    oRequest.dwServiceId=htonl(pHeader->dwServiceId);
    oRequest.dwMsgId=htonl(pHeader->dwMsgId);
	gettimeofday_a(&(oRequest.starttime),0);

    m_mapReceive[oRequest.dwSequence]=oRequest;
}
void CSapConnection::RemoveReceiveRequestHistory(const void * pBuffer)
{
    SSapMsgHeader *pHeader=(SSapMsgHeader *)(pBuffer);
    MHistory::iterator itr=m_mapReceive.find(htonl(pHeader->dwSequence));
    if(itr!=m_mapReceive.end())
    {
        SSapRequestHistory & oRequest=itr->second;
        struct tm tmStart;
        struct tm tmEnd;
		
		struct timeval_a now;
		gettimeofday_a(&now,0);
		
		/*localtime_r(&(oRequest.starttime.tv_sec),&tmStart);
		localtime_r(&(now.tv_sec),&tmEnd);*/
		ptime pStart = from_time_t(oRequest.starttime.tv_sec);
		tmStart = to_tm(pStart);

		ptime pEnd = second_clock::local_time();
		tmEnd = to_tm(pEnd);
		
		char szTmp[1024]={0};
		sprintf(szTmp,"%04u-%02u-%02u %02u:%02u:%02u.%03u",tmStart.tm_year+1900,tmStart.tm_mon+1,tmStart.tm_mday,tmStart.tm_hour,tmStart.tm_min,tmStart.tm_sec,(unsigned int)(oRequest.starttime.tv_usec/1000));
		string strStartTime = szTmp;

		memset(szTmp,0,1024);
		sprintf(szTmp,"%04u-%02u-%02u %02u:%02u:%02u.%03u",tmEnd.tm_year+1900,tmEnd.tm_mon+1,tmEnd.tm_mday,tmEnd.tm_hour,tmEnd.tm_min,tmEnd.tm_sec,(unsigned int)(now.tv_usec/1000));
		string strEndTime = szTmp;

        struct timeval_a tmInterval;
        timersub(&now,&(oRequest.starttime),&tmInterval);
		
		//LOG:nid,serviceid,msgid,sequence,starttime,interval(ms), IN
		SP_XLOG(XLOG_NOTICE,"%u,%u,%u,%u,%s,%s,%d.%03d,%d,IN\n",oRequest.nId,oRequest.dwServiceId,oRequest.dwMsgId,oRequest.dwSequence,strStartTime.c_str(),strEndTime.c_str(),tmInterval.tv_sec*1000+tmInterval.tv_usec/1000,tmInterval.tv_usec%1000,ntohl(pHeader->dwCode));

        m_mapReceive.erase(itr);
    }
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
    AsyncWrite(&response,sizeof(SSapMsgHeader));
}

}

}

