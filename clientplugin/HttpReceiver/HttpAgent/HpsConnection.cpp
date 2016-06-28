#include <boost/bind.hpp>
#include <iostream>
#include <istream>
#include <ostream>

#include "HpsConnection.h"
#include "HpsLogHelper.h"
#include "HpsResponseMsg.h"
#include "HpsCommon.h"
#include "HpsSession.h"
#include "CommonMacro.h"

#ifdef WIN32
#ifndef strncasecmp
#define strncasecmp strnicmp 
#endif

#endif

unsigned int CHpsConnection::sm_dwMark=0;
		
CHpsConnection::CHpsConnection(boost::asio::io_service &io_service, IHttpVirtualClient *pHttpVirtualClient):
	m_oIoService(io_service),m_socket(io_service),m_dwId(sm_dwMark++),m_nRemotePort(0),m_bRunning(true),m_pSession(NULL),m_isKeepAlive(false),
        m_pHttpVirtualClient(pHttpVirtualClient)
{
	HP_XLOG(XLOG_DEBUG,"----------------------------CHpsConnection()----------m_nId[%u]----------------\n",m_dwId);
}

CHpsConnection::~CHpsConnection()
{
	HP_XLOG(XLOG_DEBUG,"----------------------------~CHpsConnection()---------m_nId[%u]----------------\n",m_dwId);
}

void CHpsConnection::AsyncRead()
{
	m_socket.get_io_service().post(boost::bind(&CHpsConnection::AsyncReadInner, shared_from_this()));
}

void CHpsConnection::AsyncReadInner()
{	
	HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s,connection[%0x]\n",__FUNCTION__,this);
    try{
        m_strRemoteIp=m_socket.remote_endpoint().address().to_string();
        m_nRemotePort=m_socket.remote_endpoint().port();
    }
    catch(boost::system::system_error & ec){
     	HP_SLOG(XLOG_WARNING,"CHpsConnection::OnReceiveRequestCompleted,code:" <<ec.what()<<"\n");
    }

	m_pHead = hpsBuffer.base();
	m_socket.async_read_some( boost::asio::buffer(hpsBuffer.top(),hpsBuffer.capacity() - 1),
			MakeHpsAllocHandler(m_allocRead,boost::bind(&CHpsConnection::HandleReadRequest, shared_from_this(),
					boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred)));
	HP_XLOG(XLOG_TRACE,"CHpsConnection::%s::%d,m_pHead[%x]\n",__FUNCTION__,__LINE__,m_pHead);
}


void CHpsConnection::HandleReadRequest(const boost::system::error_code& err,std::size_t dwTransferred)
{
	HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s,connection[%0x],bytes_transferred[%d],hpsBuffer.len[%d]\n",__FUNCTION__,this,dwTransferred,hpsBuffer.len());
    if(!err)
    {
        hpsBuffer.inc_loc(dwTransferred);		
		HP_XLOG(XLOG_TRACE,"CHpsConnection::%s::%d,m_pHead[0X%0X],hpsBuffer.base[0X%0X],hpsBuffer.len[%d]\n",__FUNCTION__,__LINE__,m_pHead,hpsBuffer.base(),hpsBuffer.len());
		if(hpsBuffer.len()>= MAX_HPS_BUFFER)
        {
            HP_SLOG(XLOG_WARNING,"CHpsConnection::"<<__FUNCTION__<<"--1--,connection["<<(void *)this<<"],error:too big packet\n");
    		OnPeerClose();	
            return;
        }
		
		hpsBuffer.SetTopZero();
		HP_XLOG(XLOG_TRACE,"CHpsConnection::%s::%d,m_pHead[0X%0X],hpsBuffer.base[0X%0X],hpsBuffer.len[%d]\n",__FUNCTION__,__LINE__,m_pHead,hpsBuffer.base(),hpsBuffer.len());

        char *pEnd = strstr(m_pHead,"\r\n\r\n");
        while(pEnd!=NULL)
        {
            if(0==strncmp(m_pHead, "GET", 3))
            {
                OnReceiveRequestCompleted(m_pHead,pEnd+4-m_pHead);
                m_pHead=pEnd+4;
            }
            else if(0==strncmp(m_pHead, "POST", 4))
            {
                char *pFound = strstr(m_pHead,"\r\n");
				int lenbody=-1;
				while( pFound != NULL && pFound < pEnd ) 
				{
					HP_XLOG(XLOG_TRACE,"CHpsConnection::%s,-------post find content-length\n",__FUNCTION__);
					if(strncasecmp(pFound,"\r\nContent-length",16)==0)
					{
						sscanf(pFound+16,"%*[: ]%d",&lenbody);
						HP_XLOG(XLOG_TRACE,"CHpsConnection::%s,-------lenbody[%d]\n",__FUNCTION__,lenbody);
						break;
					}
					pFound = strstr(pFound+2,"\r\n");
				}
                if(lenbody!=-1)
                {                   	
					HP_XLOG(XLOG_TRACE,"CHpsConnection::%s::%d,pHead[0X%0X],hpsBuffer.base[0X%0X],pEnd[0X%0X],hpsBuffer.len[%d],content-length[%d]\n",__FUNCTION__,__LINE__,
						m_pHead,hpsBuffer.base(),pEnd,hpsBuffer.len(),lenbody);
                	if(hpsBuffer.top()>=pEnd+4+lenbody)
                	{
                    	OnReceiveRequestCompleted(m_pHead,pEnd+4+lenbody-m_pHead);
                    	m_pHead=pEnd+4+lenbody;						
						HP_XLOG(XLOG_TRACE,"CHpsConnection::%s::%d,m_pHead[0X%0X]\n",__FUNCTION__,__LINE__,m_pHead);
                	}
					else
					{
						HP_XLOG(XLOG_TRACE,"CHpsConnection::%s,incomplement packet\n",__FUNCTION__);
						break;
					}
					
                }
				else
				{
					HP_XLOG(XLOG_TRACE,"CHpsConnection::%s,no Content-length\n",__FUNCTION__);
					OnPeerClose();	
		            return;
				}
            }
            else
            {
                HP_XLOG(XLOG_WARNING,"CHpsConnection::%s,connection[0X%0X],str[%s]\n",__FUNCTION__,this,m_pHead);									
				DoSendResponse(false, 0, ERROR_HEAD_NOT_IMPLEMENTED);
				OnPeerClose();
				return ;
            }
            pEnd = strstr(m_pHead,"\r\n\r\n");
        }
		HP_XLOG(XLOG_TRACE,"CHpsConnection::%s::%d, m_pHead[0X%0X],hpsBuffer.base[0X%0X],hpsBuffer.len[%d]\n",__FUNCTION__,__LINE__,m_pHead,hpsBuffer.base(),hpsBuffer.len());
        if(m_pHead!=hpsBuffer.base())
        {
			HP_XLOG(XLOG_TRACE,"CHpsConnection::%s::%d, read packet over, memmove the packet\n",__FUNCTION__,__LINE__);
			memmove(hpsBuffer.base(),m_pHead,hpsBuffer.top()-m_pHead);
			hpsBuffer.reset_loc(hpsBuffer.top() - m_pHead);
			m_pHead = hpsBuffer.base();				
			HP_XLOG(XLOG_TRACE,"CHpsConnection::%s::%d, hpsBuffer.base[0X%0X], hpsBuffer.len[%d], hpsBuffer.capacity[%d]\n",
				__FUNCTION__,__LINE__,hpsBuffer.base(),hpsBuffer.len(),hpsBuffer.capacity());
        }
		if(hpsBuffer.capacity()-1 == 0)
		{		
			HP_XLOG(XLOG_TRACE,"CHpsConnection::%s, increase 4096 bytes\n",__FUNCTION__);
            hpsBuffer.add_capacity(4096);
			m_pHead = hpsBuffer.base();
		}
		
        m_socket.async_read_some(boost::asio::buffer(hpsBuffer.top(),hpsBuffer.capacity() - 1),
            MakeHpsAllocHandler(m_allocRead,boost::bind(&CHpsConnection::HandleReadRequest
						, shared_from_this(), boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred)));  
    }
    else
	{
		HP_SLOG(XLOG_DEBUG,"CHpsConnection::"<<__FUNCTION__<<"--2--,connection["<<(void *)this<<"],error:" <<err.message()<<"\n");
		OnPeerClose();
	}
}


void CHpsConnection::OnReceiveRequestCompleted(char * pBuff, int nLen)
{
	HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s,connection[%0x],nLen[%d],request:\n%s\n",__FUNCTION__,this,nLen,pBuff);
	if( NULL != m_pSession)
	{
		m_pSession->OnReceiveHttpRequest(pBuff,nLen,m_strRemoteIp, m_nRemotePort);
	}
}

void CHpsConnection::DoSendResponse(bool isKeepAlive, int nHttpVersion, int nCode)
{	
	HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s,connection[%0x],isKeepAlive[%d], nHttpVersion[%d], nCode[%d]\n",
		__FUNCTION__,this,isKeepAlive,nHttpVersion,nCode);

	m_isKeepAlive = isKeepAlive;
        
        CHpsResponseMsg resMsg;
        if (NULL != m_pHttpVirtualClient)
        {
            HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s, Response to virtual client\n", __FUNCTION__);
            m_pHttpVirtualClient->OnPeerResponse(m_dwId, resMsg.EncodeBody(nCode));
            return;
        }
        
		
	string strRes = resMsg.Encode(nHttpVersion,isKeepAlive,nCode);

	HandleSendResponse(strRes);
}

void CHpsConnection::DoSendResponse(bool isKeepAlive, int nHttpVersion, const string &strCharSet,
				const string &strContentType, const string &strHttpResponse, int nHttpCode,
				const vector<string>& vecCookie, const string& strLocationAddr, 
				const string &strHeaders)
{	
	HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s,connection[%0x],isKeepAlive[%d], nHttpVersion[%d], response[%s]\n",
		__FUNCTION__,this,isKeepAlive,nHttpVersion,strHttpResponse.c_str());

	m_isKeepAlive = isKeepAlive;
        
        if (NULL != m_pHttpVirtualClient)
        {
            HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s, Response to virtual client\n", __FUNCTION__);
            m_pHttpVirtualClient->OnPeerResponse(m_dwId, strHttpResponse);
            return;
        }
        
	CHpsResponseMsg resMsg;	
	string strRes = resMsg.Encode(nHttpVersion,isKeepAlive,strCharSet,
		strContentType,strHttpResponse,nHttpCode,vecCookie,strLocationAddr,strHeaders);

	HandleSendResponse(strRes);
}


void CHpsConnection::DoSendResponse(bool isKeepAlive, const string &strHttpResponse)
{	
	HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s,connection[%0x],isKeepAlive[%d]\n",__FUNCTION__,this,isKeepAlive);

	m_isKeepAlive = isKeepAlive;
	HandleSendResponse(strHttpResponse);
}


void CHpsConnection::HandleSendResponse(const string &strResponse)
{
	HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s,connection[%0x],response\n%s\n",__FUNCTION__,this,strResponse.c_str());

	SHpsLenMsg msg;
	msg.nLen = (int)(strResponse.size());
	msg.pBuffer = (void*)malloc(msg.nLen);
	memcpy(msg.pBuffer, strResponse.c_str(), msg.nLen);
	
	bool write_in_progress = !m_queueOut.empty();
    m_queueOut.push_back(msg);
    if (!write_in_progress)
    {
        boost::asio::async_write(m_socket, boost::asio::buffer(m_queueOut.front().pBuffer,m_queueOut.front().nLen),
        	MakeHpsAllocHandler(m_allocWrite,boost::bind(&CHpsConnection::HandleWriteResponse, shared_from_this(),
        		boost::asio::placeholders::error)));
    }
}


void CHpsConnection::HandleWriteResponse(const boost::system::error_code& err)
{
	HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s,connection[%0x]\n",__FUNCTION__,this);
	if (!err)
	{
		free((void *)(m_queueOut.front().pBuffer));
   		m_queueOut.pop_front();
    	if (!m_queueOut.empty())
    	{
        	boost::asio::async_write(m_socket, boost::asio::buffer(m_queueOut.front().pBuffer,m_queueOut.front().nLen),
        		MakeHpsAllocHandler(m_allocWrite,boost::bind(&CHpsConnection::HandleWriteResponse, shared_from_this(),
        			boost::asio::placeholders::error)));
    	}

		if(!m_isKeepAlive&&m_queueOut.empty())
		{
                    HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s, Close[%d]\n",__FUNCTION__, m_dwId);
			OnPeerClose();
		}
	}
	else
	{	
		while (!m_queueOut.empty())
		{
			free((void *)(m_queueOut.front().pBuffer));
			m_queueOut.pop_front();
		}

		OnPeerClose();
		HP_SLOG(XLOG_WARNING,"CHpsConnection::"<<__FUNCTION__<<",connection["<<(void *)this<<"],error:" 
			<<err.message()<<"\n");
	}
}

void CHpsConnection::OnReceiveVirtalClientRequest(char *pBuff, unsigned int nLen)
{
    HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s, pBuff[%0x], Len[%d]\n",__FUNCTION__, pBuff, nLen);
    hpsBuffer.reset_loc(0);
    if (nLen > hpsBuffer.capacity())
    {
        hpsBuffer.add_capacity(nLen-hpsBuffer.capacity());
    }
    
    memcpy(hpsBuffer.base(), pBuff, nLen);
    hpsBuffer.reset_loc(nLen);
    
    m_oIoService.post(boost::bind(&CHpsConnection::DoReceiveVirtalClientRequest,this));
}

void CHpsConnection::DoReceiveVirtalClientRequest()
{
    HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s\n",__FUNCTION__);
    OnReceiveRequestCompleted(hpsBuffer.base(), hpsBuffer.len());
}

void CHpsConnection::OnVirtalClientClosed()
{
    HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s ID[%d]\n", __FUNCTION__, m_dwId);
    m_oIoService.post(boost::bind(&CHpsConnection::OnPeerClose, this));
}
	
void CHpsConnection::OnPeerClose()
{
	HP_XLOG(XLOG_DEBUG,"CHpsConnection::%s,connection[%0x] id[%d]\n", __FUNCTION__, this, m_dwId);

	
	boost::system::error_code ignore_ec;
	//HP_XLOG(XLOG_ERROR,"CHpsConnection::%s,m_dwId[%d]\n",__FUNCTION__,m_dwId);
        bool bRunning = m_bRunning;
        m_bRunning = false;
        
	m_socket.close(ignore_ec);
        
        if (NULL != m_pHttpVirtualClient)
        {
            m_pHttpVirtualClient->OnPeerClose(m_dwId);
        }
	
	if(bRunning && NULL != m_pSession)
	{
		m_pSession->OnHttpPeerClose(m_strRemoteIp,m_nRemotePort);
	}
}

void CHpsConnection::Dump()
{
	HP_XLOG(XLOG_NOTICE,"  ==============CHpsConnection::%s,m_dwId[%u]]=========\n",__FUNCTION__,m_dwId);
	HP_XLOG(XLOG_NOTICE,"           m_strRemoteIp[%s], m_nRemotePort[%u]\n", m_strRemoteIp.c_str(),m_nRemotePort);
	HP_XLOG(XLOG_NOTICE,"  ==============CHpsConnection::%s==End==============================\n",__FUNCTION__);
}


