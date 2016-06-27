#include <boost/bind.hpp>
#include <boost/exception/all.hpp>
#include "HttpsConnection.h"
#include "HTDealerServiceLog.h"
#include "HttpsClientImp.h"
//using sdo::coh::CCohHandlerAllocator;
using sdo::coh::MakeCohAllocHandler;

static const unsigned int MAX_HTTP_CLIENT_BUFFER=1024*1024;
int CHttpsConnection::s_Timeout = 5;
CHttpsConnection::CHttpsConnection(CHttpsClientImp* pClient, 
    boost::asio::io_service& io_service, 
    context& context)
  :pClient_(pClient),
  socket_(io_service, context),resolver_(io_service),timer_(io_service)
{}

void CHttpsConnection::StartSendRequest(const string& hostName, const string& request)
{
    HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s host[%s] request[%s]\n", 
        __FUNCTION__, hostName.c_str(), request.c_str());
    requestData_ = request;
	try
	{
		
		tcp::resolver::query query(hostName, "https");		
		tcp::resolver::iterator iterator = resolver_.resolve(query);		
		resolver_.async_resolve(query,
			MakeCohAllocHandler(allocConn_,boost::bind(&CHttpsConnection::HandleResolve, shared_from_this(),
									boost::asio::placeholders::error,
									boost::asio::placeholders::iterator)));		
    }
	catch(boost::exception &err)
	{
		HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s 域名解析错误\n", __FUNCTION__);
		rspContent_  = "HTTP/1.1 555 ";
        rspContent_ += "域名解析错误";
        rspContent_ += "\r\n\r\n";		
		OnReceiveCompleted(rspContent_);
		return;
	}
	
	timer_.expires_from_now(boost::posix_time::seconds(s_Timeout));	
	timer_.async_wait(MakeCohAllocHandler(allocTimer_,boost::bind(&CHttpsConnection::HandleResponseTimeout, 
					shared_from_this(),boost::asio::placeholders::error)));
	

}

void CHttpsConnection::HandleResolve(const boost::system::error_code& err,
    tcp::resolver::iterator endpoint_iterator)
{
	 HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s >>>\n",__FUNCTION__);
	if (!err)
	{
		tcp::endpoint endpoint = *endpoint_iterator;
		HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s,addr[%s:%d]\n",__FUNCTION__,
          endpoint.address().to_string().c_str(), endpoint.port());
		  
		socket_.lowest_layer().async_connect(endpoint,
				MakeCohAllocHandler(allocConn_,
				    boost::bind(&CHttpsConnection::HandleConnect, 
				        shared_from_this(),
						boost::asio::placeholders::error,
						++endpoint_iterator)));
	}
	else
	{
		rspContent_  = "HTTP/1.1 551 ";
        rspContent_ += err.message();
        rspContent_ += "\r\n\r\n";		
		OnReceiveCompleted(rspContent_);
	}
}

void CHttpsConnection::HandleConnect(const boost::system::error_code& error,
  tcp::resolver::iterator endpoint_iterator)
{
    HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s  >>>\n", __FUNCTION__);
    if (!error)
    {
        socket_.async_handshake(boost::asio::ssl::stream_base::client,
          MakeCohAllocHandler(allocConn_, boost::bind(&CHttpsConnection::HandleHandshake, shared_from_this(),
            boost::asio::placeholders::error)));
    }
    else if (endpoint_iterator != tcp::resolver::iterator())
    {
        boost::system::error_code ignore_ec;
        socket_.lowest_layer().close(ignore_ec);
        tcp::endpoint endpoint = *endpoint_iterator;
        socket_.lowest_layer().async_connect(endpoint,
            MakeCohAllocHandler(allocConn_,
				    boost::bind(&CHttpsConnection::HandleConnect, 
				        shared_from_this(),
						boost::asio::placeholders::error,
						++endpoint_iterator)));
    }
    else
    {
        rspContent_  = "HTTP/1.1 552 ";
        rspContent_ += error.message();
        rspContent_ += "\r\n\r\n";		
		OnReceiveCompleted(rspContent_);
    }
	HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s  <<<\n", __FUNCTION__);
}

void CHttpsConnection::HandleHandshake(const boost::system::error_code& error)
{
    HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s >>>\n", __FUNCTION__);
    if (!error)
    {
        boost::asio::async_write(socket_,
            boost::asio::buffer(requestData_),
            MakeCohAllocHandler(allocConn_, 
                boost::bind(&CHttpsConnection::HandleWriteRequest, shared_from_this(),
                boost::asio::placeholders::error)));
    }
    else
    {
        rspContent_ = "HTTP/1.1 553 ";
        rspContent_ += error.message();
        rspContent_ += "\r\n\r\n";		
		OnReceiveCompleted(rspContent_);
    }
	 HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s <<<\n", __FUNCTION__);
}

void CHttpsConnection::HandleWriteRequest(const boost::system::error_code& error)
{
    HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s >>>\n", __FUNCTION__);
    if (!error)
    {
        pHead_ = response_.base();
        socket_.async_read_some(boost::asio::buffer(response_.top(),response_.capacity() - 1),             
            MakeCohAllocHandler(allocConn_, boost::bind(&CHttpsConnection::HandleReadResponse, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)));
    }
    else
    {
        rspContent_  = "HTTP/1.1 554 ";
        rspContent_ += error.message();
        rspContent_ += "\r\n\r\n";		
		OnReceiveCompleted(rspContent_);
    }
	 HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s <<<\n", __FUNCTION__);
}


void CHttpsConnection::HandleReadResponse(const boost::system::error_code& error,
  size_t bytes_transferred)
{
    HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s %d\n", __FUNCTION__,response_.len());
    if(!error)
    {	
		int nCurrentLen = response_.len() + bytes_transferred;
		if (response_.len()>0)
		{
			HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s YYYYY\n", __FUNCTION__);
		}
        response_.inc_loc(bytes_transferred);		
		if(response_.len() >= MAX_HTTP_CLIENT_BUFFER)
        {
    		OnReceiveCompleted("HTTP/1.1 555 too big packet \r\n\r\n");
			HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s <<<\n", __FUNCTION__);
            return;
        }

        response_.SetTopZero();
        char *pEnd = strstr(pHead_, "\r\n\r\n");
        while(pEnd!=NULL)
        {
			if(0 == strncmp(pHead_, "HTTP", 4))
            {                
				int nBodyLen = -1;
				bool bHaveBody = false;
				char szHead[2560]={0};
				char szHeadValue[5120]={0};
                
	            char *pFound = strstr(pHead_, "\r\n");
				while( pFound != NULL && pFound < pEnd ) 
				{
					if(sscanf(pFound+2,"%63[^:]: %511[^\r\n]",szHead,szHeadValue)<2)
					{
						pFound = strstr(pFound+2,"\r\n");
						continue;
					}

					if(0 == strncasecmp(szHead,"Content-Length",14))
					{						
						nBodyLen  = atoi(szHeadValue);
						bHaveBody = true;
						break;
					}
					else if((0==strncasecmp(szHead,"Transfer-Encoding",17))&&
                     (0==strncasecmp(szHeadValue,"chunked",7)))
					{						
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

				if(!bHaveBody)
				{
                    rspContent_.assign(pHead_, pEnd + 4 - pHead_);
					OnReceiveCompleted(rspContent_);
					pHead_ = pEnd+4;
				}
				else
				{					
					if(nBodyLen > 0)
					{	
						if (nCurrentLen== pEnd+4+nBodyLen-pHead_)
						{
							rspContent_.assign(pHead_, pEnd+4+nBodyLen-pHead_);
							OnReceiveCompleted(rspContent_);
							pHead_ = pEnd+4+nBodyLen;
						}
						else
						{
							pHead_+=bytes_transferred;
							break;
						}

					}
					else
					{
						break;
					}					
				}
            }
            else
            {
                HT_XLOG(XLOG_WARNING,"CHttpsConnection::%s, Not HTTP Response\n",__FUNCTION__);									
				break;
            }
            pEnd = strstr(pHead_,"\r\n\r\n");
        }
		
       // if(pHead_ != response_.base())
       // {
	   //	memmove(response_.base(), pHead_, response_.top()-pHead_);
	  //	response_.reset_loc(response_.top() - pHead_);
		//	pHead_ = response_.base();				
       // }
		
		if(response_.capacity()-1 == 0)
		{		
            response_.add_capacity(4096*2);
			pHead_ = response_.base();
		}	
		pHead_ = response_.base();
    	socket_.async_read_some(boost::asio::buffer(response_.top(),response_.capacity() - 1),             
            MakeCohAllocHandler(allocConn_, boost::bind(&CHttpsConnection::HandleReadResponse, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)));
    }
    else if (error != boost::asio::error::operation_aborted)
    {
        boost::system::error_code ignore_ec;
		socket_.lowest_layer().close(ignore_ec);
    }
	HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s <<<\n", __FUNCTION__);
}



void CHttpsConnection::HandleResponseTimeout(const boost::system::error_code& err)
{
	HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s\n", __FUNCTION__);
	if(err != boost::asio::error::operation_aborted)
	{
		boost::system::error_code ignore_ec;
		socket_.lowest_layer().close(ignore_ec);
	}
}

void CHttpsConnection::OnReceiveCompleted(const string& response)
{
    boost::system::error_code ignore_ec;
    timer_.cancel(ignore_ec);
    if(pClient_ != NULL)
    {
        HT_XLOG(XLOG_DEBUG,"CHttpsConnection::%s,response:\n%s\n\n", __FUNCTION__, response.c_str());    	
    	pClient_->OnReceiveResponse(response);
    }
}
