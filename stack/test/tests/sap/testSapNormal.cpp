#include <boost/test/unit_test.hpp>
#include "SapRequestHistory.h"
#include <string>
#include "ISapCallback.h"
#include "SapAgent.h"
#include "SapStack.h"
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "SapLogHelper.h"
/*
using boost::asio::ip::tcp;

using std::string;
using namespace sdo::sap;
static int sg_nSapPort=3444;

static int nIdentifier=0;
class MySapCallback2:public ISapCallback
{
public:
    virtual void OnConnectResult(int nId,int nResult)
    {
    }
	virtual void OnReceiveConnection(int nId,const string &strRemoteIp,unsigned int dwRemotPort)
    {
        nIdentifier+=1;
    }
	virtual void OnReceiveMessage(int nId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotPort)
    {
        nIdentifier+=5;
    }
	virtual void OnPeerClose(int nId)
    {
        nIdentifier+=3;
    }
	virtual ~MySapCallback2(){}
};

struct testSapFixture
{
    testSapFixture()
    {
        //XLOG_INIT("log.properties",true);
        //XLOG_REGISTER(SAP_STACK_MODULE,"sap_stack");
        nIdentifier=0;
        GetSapStackInstance()->Start();
        CSapAgent::Instance()->SetCallback(&cb);
        while(CSapAgent::Instance()->StartServer(sg_nSapPort)!=0)
        {
            sg_nSapPort++;
        }
    }
    ~testSapFixture()
    {
        CSapAgent::Instance()->StopServer();
        CSapAgent::Instance()->SetCallback(NULL);
        GetSapStackInstance()->Stop();
    }
    MySapCallback2 cb;
};
BOOST_FIXTURE_TEST_SUITE(test_sap_normal, testSapFixture )
BOOST_AUTO_TEST_CASE(test_sap_connect)
{
    CSapAgent::Instance()->SetInterval(2,1,1);

    char szPort[16]={0};
    sprintf(szPort,"%d",sg_nSapPort);
    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query("127.0.0.1",szPort);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    tcp::socket socket(io_service);
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,2,100000));
    BOOST_CHECK_EQUAL(nIdentifier,4);
    
}
BOOST_AUTO_TEST_CASE(test_sap_client_connect)
{
    CSapAgent::Instance()->SetInterval(2,1,2);

    int id=CSapAgent::Instance()->DoConnect("127.0.0.1",sg_nSapPort,1);
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,0,100000));
    BOOST_CHECK_EQUAL(nIdentifier,1);
    CSapAgent::Instance()->DoClose(id);
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,0,100000));
    BOOST_CHECK_EQUAL(nIdentifier,4);
}
BOOST_AUTO_TEST_CASE(test_sap_send_message)
{
    CSapAgent::Instance()->SetInterval(1,1,2);

    int id=CSapAgent::Instance()->DoConnect("127.0.0.1",sg_nSapPort,1);
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,0,100000));
    BOOST_CHECK_EQUAL(nIdentifier,1);
    
    CSapEncoder msg(SAP_PACKET_REQUEST,0,0,0);
    msg.SetSequence(1);
    
    CSapAgent::Instance()->DoSendMessage(id,msg.GetBuffer(),msg.GetLen());
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,0,100000));
    BOOST_CHECK_EQUAL(nIdentifier,6);
    
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,3,100000));
    BOOST_CHECK_EQUAL(nIdentifier,11);
    CSapAgent::Instance()->DoClose(id);
    
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,0,100000));
    BOOST_CHECK_EQUAL(nIdentifier,14);
}
BOOST_AUTO_TEST_SUITE_END()
*/
