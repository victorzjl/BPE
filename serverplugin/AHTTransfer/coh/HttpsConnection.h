#ifndef __HTTPS_CONNENCTION_H__
#define __HTTPS_CONNENCTION_H__
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "CohHandlerAlloc.h"
#include "HttpBuffer.h"
#include "HttpClientBase.h"

#include "SmallObject.h"


using std::string;
using boost::asio::ip::tcp;
using boost::asio::ssl::context;
using namespace HT_COH;

class CHttpsClientImp;
class CHttpsConnection:public sdo::common::CSmallObject,
    public boost::enable_shared_from_this<CHttpsConnection>,
    private boost::noncopyable
{
public:
  CHttpsConnection(CHttpsClientImp* pClient, boost::asio::io_service& io_service, context& context);
  void StartSendRequest(const string& hostName, const string& request);
  static void SetTimeout(int nTimeout){s_Timeout=nTimeout;}
private:
    void HandleResolve(const boost::system::error_code& err,
    tcp::resolver::iterator endpoint_iterator);
  void HandleConnect(const boost::system::error_code& error,
      tcp::resolver::iterator endpoint_iterator);
  void HandleHandshake(const boost::system::error_code& error); 
  void HandleWriteRequest(const boost::system::error_code& error);
  void HandleReadResponse(const boost::system::error_code& error, size_t bytes_transferred);
  void OnReceiveCompleted(const string& response);
  void HandleResponseTimeout(const boost::system::error_code& err);

private:
    static const int MAX_LENGTH = 4*1024;
    static int s_Timeout;
    CHttpsClientImp *pClient_;
    boost::asio::ssl::stream<tcp::socket> socket_;
    tcp::resolver resolver_;
    sdo::coh::CCohHandlerAllocator allocConn_;
    sdo::coh::CCohHandlerAllocator allocTimer_;
    boost::asio::deadline_timer timer_;    
    string requestData_;
    string rspContent_;
    char* pHead_;
    CHttpBuffer response_;
    
};

typedef boost::shared_ptr<CHttpsConnection> HttpsConn_ptr;

#endif
