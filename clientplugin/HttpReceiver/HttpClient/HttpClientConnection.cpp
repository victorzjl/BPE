#include <boost/bind.hpp>
#include <iostream>
#include <istream>
#include <ostream>

#include "HttpClientConnection.h"
#include "HttpClientLogHelper.h"
#include "HttpClientSession.h"

#ifdef WIN32
#ifndef strncasecmp
#define strncasecmp strnicmp 
#endif

#endif

CHttpClientConnection::CHttpClientConnection(boost::asio::io_service &io_service,CHttpClientSession *pSession):
	m_oIoService(io_service),m_socket(io_service),m_resolver(io_service),m_connTimer(io_service),m_pSession(pSession)
{
}

CHttpClientConnection::~CHttpClientConnection()
{
}

void CHttpClientConnection::AsyncConnectToServer(const string &strIp,unsigned int dwPort,int nTimeout)
{
    HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientConnection::async_connect,addr[%s:%d],timeout[%d]\n",strIp.c_str(),dwPort,nTimeout);
    m_isConnected=false;
    
    char szBuf[16]={0};
    sprintf(szBuf,"%d",dwPort);
    tcp::resolver::query query(strIp,szBuf);
    m_resolver.async_resolve(query,
        MakeHttpClientAllocHandler(m_allocConn, boost::bind(&CHttpClientConnection::HandleResolve, shared_from_this(),
              boost::asio::placeholders::error, boost::asio::placeholders::iterator)));

	m_connTimer.expires_from_now(boost::posix_time::seconds(nTimeout));
	m_connTimer.async_wait(MakeHttpClientAllocHandler(m_allocTimer,boost::bind(&CHttpClientConnection::HandleConnectTimeout, 
					shared_from_this(),boost::asio::placeholders::error)));
}

void CHttpClientConnection::HandleResolve(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator)
{
    if (!err)
    {
        tcp::endpoint endpoint = *endpoint_iterator;
        HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientConnection::%s,addr[%s:%d]\n",__FUNCTION__,endpoint.address().to_string() .c_str() ,endpoint.port());
        m_socket.async_connect(endpoint,
            MakeHttpClientAllocHandler(m_allocConn, boost::bind(&CHttpClientConnection::HandleConnected, shared_from_this(),
            	boost::asio::placeholders::error, ++endpoint_iterator)));
    }
    else
    {
      HTTPCLIENT_SLOG(XLOG_WARNING,"CHttpClientConnection::"<<__FUNCTION__<<",this["<<this<<"],error:" <<err.message()<<"\n");
      ConnectFinish(-2);
    }
}


void CHttpClientConnection::HandleConnected(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator)
{
    HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientConnection::%s,this[%0X]\n",__FUNCTION__,this);
    if (!err)
    {
        ConnectFinish(0);
    }
    else if (endpoint_iterator != tcp::resolver::iterator())
    {
      boost::system::error_code ignore_ec;
      m_socket.close(ignore_ec);
      tcp::endpoint endpoint = *endpoint_iterator;
      m_socket.async_connect(endpoint,
          MakeHttpClientAllocHandler(m_allocConn,boost::bind(&CHttpClientConnection::HandleConnected, shared_from_this(),
            boost::asio::placeholders::error, ++endpoint_iterator)));
    }
    else
    {
		HTTPCLIENT_XLOG(XLOG_WARNING,"CHttpClientConnection::%s, this[%0X],  error:%s\n",	__FUNCTION__,this,(err.message()).c_str());
        ConnectFinish(-3);
    }
}

void CHttpClientConnection::HandleConnectTimeout(const boost::system::error_code& err)
{
	HTTPCLIENT_SLOG(XLOG_DEBUG,"CHttpClientConnection::"<<__FUNCTION__<<",connection["<<(void *)this<<"],code:" <<err.message()<<"\n");
	if(err!=boost::asio::error::operation_aborted)
	{
		boost::system::error_code ignore_ec;
		m_socket.close(ignore_ec);

		ConnectFinish(-3);
	}
}


void CHttpClientConnection::ConnectFinish(int nResult)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientConnection::%s,nResult[%d]\n",__FUNCTION__,nResult);
	boost::system::error_code ignore_ec;
	m_connTimer.cancel(ignore_ec);
	
	if(nResult == 0)
	{
		m_isConnected = true;
		StartRead();
	}
	
	if(m_pSession != NULL)
	{
		m_pSession->ConnectResult(nResult);
	}
}

void CHttpClientConnection::SendHttpRequest(const string &strRequest)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientConnection::%s,strRequest[\n%s\n]\n",__FUNCTION__,strRequest.c_str());
	SHttpClientMsg msg;
	msg.nLen = (int)(strRequest.size());
	msg.pBuffer = (void*)malloc(msg.nLen);
	memcpy(msg.pBuffer, strRequest.c_str(), msg.nLen);
	
	bool write_in_progress = !m_queueOut.empty();
    m_queueOut.push_back(msg);
    if (!write_in_progress)
    {
        boost::asio::async_write(m_socket, boost::asio::buffer(m_queueOut.front().pBuffer,m_queueOut.front().nLen),
        	MakeHttpClientAllocHandler(m_allocWriter,boost::bind(&CHttpClientConnection::HandleSendRequest, shared_from_this(),
        		boost::asio::placeholders::error)));
    }
}



void CHttpClientConnection::HandleSendRequest(const boost::system::error_code& err)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientConnection::%s,connection[%0x]\n",__FUNCTION__,this);
	if (!err)
	{
		free((void *)(m_queueOut.front().pBuffer));
   		m_queueOut.pop_front();
    	if (!m_queueOut.empty())
    	{
        	boost::asio::async_write(m_socket, boost::asio::buffer(m_queueOut.front().pBuffer,m_queueOut.front().nLen),
        		MakeHttpClientAllocHandler(m_allocWriter,boost::bind(&CHttpClientConnection::HandleSendRequest, shared_from_this(),
        			boost::asio::placeholders::error)));
    	}
	}
	else
	{
		while (!m_queueOut.empty())
		{
			free((void *)(m_queueOut.front().pBuffer));
			m_queueOut.pop_front();
		}
		HTTPCLIENT_SLOG(XLOG_WARNING,"CHttpClientConnection::"<<__FUNCTION__<<",connection["<<(void *)this<<"],error:" 
			<<err.message()<<"\n");
			
		if(m_pSession != NULL)
		{
			m_pSession->SendHttpRequestFailed(HTTP_CODE_SEND_REQUEST_FAIL,ERROR_SEND_HTTP_REQUEST_FAIL);
		}
	}
}

void CHttpClientConnection::StartRead()
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientConnection::%s,connection[%0x]\n",__FUNCTION__,this);

	m_pHead = m_buffer.base();
	m_socket.async_read_some( boost::asio::buffer(m_buffer.top(),m_buffer.capacity() - 1),
			MakeHttpClientAllocHandler(m_allocReader,boost::bind(&CHttpClientConnection::HandleReadResponse, shared_from_this(),
					boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred)));
	HTTPCLIENT_XLOG(XLOG_TRACE,"CHttpClientConnection::%s::%d,m_pHead[%x]\n",__FUNCTION__,__LINE__,m_pHead);
}

void CHttpClientConnection::HandleReadResponse(const boost::system::error_code& err,std::size_t dwTransferred)
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientConnection::%s,connection[%0x],bytes_transferred[%d]\n",__FUNCTION__,this,dwTransferred);
    if(!err)
    {
        m_buffer.inc_loc(dwTransferred);		
		if(m_buffer.len() >= MAX_HTTP_CLIENT_BUFFER)
        {
            HTTPCLIENT_SLOG(XLOG_WARNING,"CHttpClientConnection::"<<__FUNCTION__<<"--1--,connection["<<(void *)this<<"],error:too big packet\n");
    		OnPeerClose();	
            return;
        }
		
		m_buffer.SetTopZero();

        char *pEnd = strstr(m_pHead,"\r\n\r\n");
        while(pEnd!=NULL)
        {
			if(0 == strncmp(m_pHead, "HTTP", 4))
            {
                char *pFound = strstr(m_pHead,"\r\n");
				int nBodyLen = -1;
				bool bHaveBody = false;

				char szHead[64]={0};
				char szHeadValue[512]={0};
	
				while( pFound != NULL && pFound < pEnd ) 
				{
					if(sscanf(pFound+2,"%63[^:]: %511[^\r\n]",szHead,szHeadValue)<2)
					{
						pFound = strstr(pFound+2,"\r\n");
						continue;
					}

					if(0==strncasecmp(szHead,"Content-Length",14))
					{
						HTTPCLIENT_XLOG(XLOG_TRACE,"CHttpClientConnection::%s::%d, content-length packet\n",__FUNCTION__,__LINE__);
						nBodyLen=atoi(szHeadValue);
						bHaveBody = true;
						break;
					}
					else if((0==strncasecmp(szHead,"Transfer-Encoding",17))&&(0==strncasecmp(szHeadValue,"chunked",7)))
					{
						HTTPCLIENT_XLOG(XLOG_TRACE,"CHttpClientConnection::%s::%d, chunked packet\n",__FUNCTION__,__LINE__);
						bHaveBody = true;
						char *pBodyEnd = strstr(pEnd+4, "\r\n0\r\n\r\n");
						if(pBodyEnd != NULL)
						{
							nBodyLen = pBodyEnd - pEnd + 3;  //3=7-4
						}
						break;
					}
					
					pFound = strstr(pFound+2,"\r\n");
				}

				if(bHaveBody==false)
				{
					int nLen = pEnd + 4 - m_pHead;
					OnReceiveResponseCompleted(m_pHead, nLen);
					m_pHead = pEnd+4;
				}
				else
				{					
					if(nBodyLen > 0)
					{
						HTTPCLIENT_XLOG(XLOG_TRACE,"CHttpClientConnection::%s::%d, m_pHead[0X%0X], pEnd[0X%0X], nBodyLen[%d]\n",__FUNCTION__,__LINE__,m_pHead,pEnd,nBodyLen);
						OnReceiveResponseCompleted(m_pHead, pEnd+4+nBodyLen-m_pHead);
						m_pHead = pEnd+4+nBodyLen;
					}
					else
					{
						break;
					}					
				}
            }
            else
            {
                HTTPCLIENT_XLOG(XLOG_WARNING,"CHttpClientConnection::%s,connection[0X%0X], Not HTTP Response -- str:\n%s\n",__FUNCTION__,this,m_pHead);									
				break;
            }
            pEnd = strstr(m_pHead,"\r\n\r\n");
        }
		
        if(m_pHead != m_buffer.base())
        {
			HTTPCLIENT_XLOG(XLOG_TRACE,"CHttpClientConnection::%s::%d, read implement packet over, memmove the packet\n",__FUNCTION__,__LINE__);
			memmove(m_buffer.base(),m_pHead,m_buffer.top()-m_pHead);
			m_buffer.reset_loc(m_buffer.top() - m_pHead);
			m_pHead = m_buffer.base();				
        }
		
		if(m_buffer.capacity()-1 == 0)
		{		
			HTTPCLIENT_XLOG(XLOG_TRACE,"CHttpClientConnection::%s, increase 4096 bytes\n",__FUNCTION__);
            m_buffer.add_capacity(4096);
			m_pHead = m_buffer.base();
		}
		
        m_socket.async_read_some(boost::asio::buffer(m_buffer.top(),m_buffer.capacity() - 1),
            MakeHttpClientAllocHandler(m_allocReader,boost::bind(&CHttpClientConnection::HandleReadResponse
						, shared_from_this(), boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred)));  
    }
    else
	{
		HTTPCLIENT_SLOG(XLOG_DEBUG,"CHttpClientConnection::"<<__FUNCTION__<<",connection["<<(void *)this<<"],error:" <<err.message()<<"\n");
		OnPeerClose();
	}
}
	
void CHttpClientConnection::OnReceiveResponseCompleted(char *pBuffer, int nLen)
{
	if( NULL != m_pSession)
	{
		m_pSession->OnReceiveHttpResponse(pBuffer,nLen);
	}
}


void CHttpClientConnection::OnPeerClose()
{
	HTTPCLIENT_XLOG(XLOG_DEBUG,"CHttpClientConnection::%s,connection[%0x]\n",__FUNCTION__,this);
	
	if( NULL != m_pSession)
	{
		m_pSession->OnPeerClose();
	}
	boost::system::error_code ignore_ec;
	m_socket.close(ignore_ec);
}


