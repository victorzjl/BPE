#include <boost/test/unit_test.hpp>
#include "SapRequestHistory.h"
#include <string>
#include "ISapCallback.h"
#include "SapAgent.h"
#include <boost/thread.hpp>
using std::string;
using namespace sdo::sap;
#include <iostream>
/*
class MySapCallback:public ISapCallback
{
public:
    virtual void OnConnectResult(int nId,int nResult){}
	virtual void OnReceiveConnection(int nId,const string &strRemoteIp,unsigned int dwRemotPort){}
	virtual void OnReceiveMessage(int nId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotPort)
    {
        BOOST_WARN_MESSAGE(1==2,"Must print here, detect execute flow!");
        
        BOOST_CHECK_EQUAL(nId,2);

        SSapMsgHeader *pHeader=(SSapMsgHeader *)pBuffer;
        BOOST_CHECK_EQUAL(pHeader->dwSequence,(unsigned int)3);
        BOOST_CHECK_EQUAL(pHeader->dwCode,SAP_CODE_STACK_TIMEOUT);
        BOOST_CHECK_EQUAL(pHeader->byIdentifer,SAP_PACKET_RESPONSE);
        BOOST_CHECK_EQUAL(ntohs(pHeader->dwPacketLen),nLen);
    }
	virtual void OnPeerClose(int nId){}
	virtual ~MySapCallback(){}
};

struct testHistoryFixture
{
    testHistoryFixture(){}
    ~testHistoryFixture(){}
    CSapMsgTimerList m_oList;
};
BOOST_FIXTURE_TEST_SUITE(test_sap_history, testHistoryFixture )
BOOST_AUTO_TEST_CASE(test_sap_history)
{
    m_oList.AddHistory(2,3,1,1,3);
    m_oList.AddHistory(3,4,1,1,3);
    BOOST_CHECK_EQUAL(m_oList.IsExistSequence(3),true);
    BOOST_CHECK_EQUAL(m_oList.IsExistSequence(2),false);

    BOOST_CHECK_EQUAL(m_oList.RemoveHistoryBySequence(2,0),false);
    BOOST_CHECK_EQUAL(m_oList.RemoveHistoryBySequence(3,0),true);
    BOOST_CHECK_EQUAL(m_oList.RemoveHistoryBySequence(3,0),false);
    BOOST_CHECK_EQUAL(m_oList.RemoveHistoryBySequence(4,0),true);
}
BOOST_AUTO_TEST_CASE(test_sap_timeout)
{
    MySapCallback cb;
    CSapAgent::Instance()->SetCallback(&cb);
    m_oList.SetRequestTimeout(1);
    
    BOOST_CHECK_EQUAL(m_oList.IsExistSequence(3),false);
    m_oList.AddHistory(2,3,1,1,3);
    BOOST_CHECK_EQUAL(m_oList.IsExistSequence(3),true);
    
    boost::this_thread::sleep(boost::posix_time::time_duration(0,0,1,100000));
    m_oList.DetectTimerList();
    BOOST_CHECK_EQUAL(m_oList.IsExistSequence(3),false);
    CSapAgent::Instance()->SetCallback(NULL);
}
BOOST_AUTO_TEST_SUITE_END()
*/
