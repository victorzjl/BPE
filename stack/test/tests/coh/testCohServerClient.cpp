#include <boost/test/unit_test.hpp>
#include "CohStack.h"
#include "CohServer.h"
#include "CohClient.h"
#include "CohRequestMsg.h"
#include <boost/thread.hpp>
#include <stdio.h>
using namespace sdo::coh;

static int sg_nServerPort=3444;
class CTestCohServer:public ICohServer
{
public:
    virtual void OnReceiveRequest(void * pIdentifer,const string &strRequest,const string &strRemoteIp,unsigned int dwRemotPort)
    {
        CCohRequestMsg msg;
        msg.Decode(strRequest);
        BOOST_CHECK_EQUAL(msg.GetCommand(),"Test");
        BOOST_CHECK_EQUAL(msg.GetAttribute("test1"),"value1");
        BOOST_CHECK_EQUAL(msg.GetAttribute("test2"),"20");
        
        vector<string> vecCoh=msg.GetAttributes("test3");
        vector<string>::iterator itr=vecCoh.begin();
        string strV1=*itr++;
        string strV2=*itr++;

        BOOST_CHECK_EQUAL(strV1,string("aa"));
        BOOST_CHECK(strV2=="bb");
        DoSendResponse(pIdentifer,"Hello World!");
    }
};
class CTestCohClient:public ICohClient
{
public:
    CTestCohClient():bState(false){}
    virtual void OnReceiveResponse(const string &strResponse)
    {
        BOOST_CHECK_EQUAL(strResponse,"Hello World!");
        bState=true;
    }
    bool State(){return bState;}
private:
    bool bState;
};
struct testCohFixture
{
    testCohFixture(){CCohStack::Start();}
    ~testCohFixture(){CCohStack::Stop();}
    CTestCohServer server;
};
BOOST_FIXTURE_TEST_SUITE(test_coh, testCohFixture )
BOOST_AUTO_TEST_CASE(test_coh)
{ 
    int ret;
    while((ret=server.Start(sg_nServerPort))!=0)
        {
            sg_nServerPort++;
        }

	printf("listen port %d\n",sg_nServerPort);
    CCohRequestMsg msg;
    char szUrl[128]={0};
    sprintf(szUrl,"http://127.0.0.1:%d/Test",sg_nServerPort);
    msg.SetUrl(szUrl);

    msg.SetAttribute("test1", "value1");
    msg.SetAttribute("test2", 20);
    msg.SetAttribute("test3","aa");
    msg.SetAttribute("test3","bb");
    
    CTestCohClient client;
    client.DoSendRequest("127.0.0.1",sg_nServerPort,msg.Encode());

    while(client.State()==false)
    {
        boost::this_thread::sleep(boost::posix_time::time_duration(0,0,0,1000));
    }
    server.Stop();
}
BOOST_AUTO_TEST_SUITE_END()

