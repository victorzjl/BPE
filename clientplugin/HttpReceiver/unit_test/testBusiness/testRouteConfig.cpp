#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp> 
#include <boost/test/unit_test_suite.hpp>
#include "HttpRequestConfigManager.h"
#include "XmlConfigParser.h"
#include "BusinessLogHelper.h"

using namespace sdo::common;

#include <iostream>
using namespace std;

struct MsgRouteBalanceConfigFixture
{
    MsgRouteBalanceConfigFixture()
	{
		cout << "MsgRouteBalanceConfigFixture()" << endl;

		map<unsigned int, map<int, string> > mapSosPriAddrsByServiceId;
		
		map<int, string> mapSosAddrByPri;
		mapSosAddrByPri[0] = "192.169.90.22:7777;1.1.1.1:1234";
		mapSosAddrByPri[1] = "192.169.90.22:9999;1.1.1.1:2345";
		mapSosAddrByPri[2] = "192.169.90.22:8888;1.1.1.1:6666";

		mapSosPriAddrsByServiceId[11] = mapSosAddrByPri;

		map<int, string> mapSosAddrByPri2;
		mapSosAddrByPri2[0] = "192.169.90.22:7777;1.1.1.1:1234";
		mapSosAddrByPri2[1] = "192.169.90.22:9999;1.1.1.1:2345";
		mapSosAddrByPri2[2] = "192.169.90.22:8888;1.1.1.1:6666";

		mapSosPriAddrsByServiceId[12] = mapSosAddrByPri2;


		CXmlConfigParser oConfigRoute;
	    if(oConfigRoute.ParseFile("conf/HttpRequestConfig.xml")!=0)
	    {
	        BS_XLOG(XLOG_ERROR, "parse oConfigRoute error:%s\n", 
	                oConfigRoute.GetErrorMessage().c_str());
	        return ;
	    }   
		
		m_oConfig.LoadConfig(oConfigRoute.GetParameters("HttpRequest"),mapSosPriAddrsByServiceId);
		
	}
    ~MsgRouteBalanceConfigFixture(){cout << "~MsgRouteBalanceConfigFixture()" << endl;}

	CHttpRequestConfigManager m_oConfig;
};


//BOOST_AUTO_TEST_SUITE(test_requestMsgDecoder)
BOOST_FIXTURE_TEST_SUITE(test_RouteBalanceConfig,MsgRouteBalanceConfigFixture)

// no body
BOOST_AUTO_TEST_CASE(test_RouteBalanceConfig1)
{
	m_oConfig.Dump();

	int nCurrentPri = m_oConfig.GetCurrentSosPri(11,100);
	BS_XLOG(XLOG_DEBUG, "------------------------current pri[%d]---------\n", nCurrentPri);

	BS_XLOG(XLOG_DEBUG, "-------------update 11,100,-10264032------1---\n", nCurrentPri);
	m_oConfig.UpdateRouteConfig(11,100,-10264032);
	m_oConfig.Dump();
	
	nCurrentPri = m_oConfig.GetCurrentSosPri(11,100);
	BS_XLOG(XLOG_DEBUG, "------------------------current pri[%d]---------\n", nCurrentPri);

	BS_XLOG(XLOG_DEBUG, "-------------update 11,100,-10242500------1---\n", nCurrentPri);
	m_oConfig.UpdateRouteConfig(11,100,-10242500);
	m_oConfig.Dump();
	
	nCurrentPri = m_oConfig.GetCurrentSosPri(11,100);
	BS_XLOG(XLOG_DEBUG, "------------------------current pri[%d]---------\n", nCurrentPri);

	BS_XLOG(XLOG_DEBUG, "-------------update 11,100,-10242500------2---\n", nCurrentPri);
	m_oConfig.UpdateRouteConfig(11,100,-10242500);
	m_oConfig.Dump();
	
	nCurrentPri = m_oConfig.GetCurrentSosPri(11,100);
	BS_XLOG(XLOG_DEBUG, "------------------------current pri[%d]---------\n", nCurrentPri);

	BS_XLOG(XLOG_DEBUG, "-------------update 11,100,-10242500---- -3---\n", nCurrentPri);
	m_oConfig.UpdateRouteConfig(11,100,-10242500);
	m_oConfig.Dump();
	
	nCurrentPri = m_oConfig.GetCurrentSosPri(11,100);
	BS_XLOG(XLOG_DEBUG, "------------------------current pri[%d]---------\n", nCurrentPri);

	BS_XLOG(XLOG_DEBUG, "-------------update 11,100,-10242500---- -4---\n", nCurrentPri);
	m_oConfig.UpdateRouteConfig(11,100,-10242500);
	m_oConfig.Dump();
	
	nCurrentPri = m_oConfig.GetCurrentSosPri(11,100);
	BS_XLOG(XLOG_DEBUG, "------------------------current pri[%d]---------\n", nCurrentPri);

	BS_XLOG(XLOG_DEBUG, "-------------update 11,100,-10242500---- -5---\n", nCurrentPri);
	m_oConfig.UpdateRouteConfig(11,100,-10242500);
	m_oConfig.Dump();
	
	nCurrentPri = m_oConfig.GetCurrentSosPri(11,100);
	BS_XLOG(XLOG_DEBUG, "------------------------current pri[%d]---------\n", nCurrentPri);


	BS_XLOG(XLOG_DEBUG, "-------------update 11,100,-10264032------2---\n", nCurrentPri);
	m_oConfig.UpdateRouteConfig(11,100,-10264032);
	m_oConfig.Dump();

	nCurrentPri = m_oConfig.GetCurrentSosPri(11,100);
	BS_XLOG(XLOG_DEBUG, "------------------------current pri[%d]---------\n", nCurrentPri);


	BS_XLOG(XLOG_DEBUG, "-------------update 11,100,-10211111------1---\n", nCurrentPri);
	m_oConfig.UpdateRouteConfig(11,100,-10211111);
	m_oConfig.Dump();

	nCurrentPri = m_oConfig.GetCurrentSosPri(11,100);
	BS_XLOG(XLOG_DEBUG, "------------------------current pri[%d]---------\n", nCurrentPri);


	BS_XLOG(XLOG_DEBUG, "-------------update 11,100,-10211111------2---\n", nCurrentPri);
	m_oConfig.UpdateRouteConfig(11,100,-10211111);
	m_oConfig.Dump();
	

}


BOOST_AUTO_TEST_CASE(test_GetResponseString)
{
	m_oConfig.Dump();
	string strRes = m_oConfig.GetResponseString(11,100,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,100,-10250001)----------------\n%s\n\n",strRes.c_str());

	strRes = m_oConfig.GetResponseString(11,101,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,101,-10250001)----------------\n%s\n\n",strRes.c_str());

	strRes = m_oConfig.GetResponseString(11,102,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,102,-10250001)----------------\n%s\n\n",strRes.c_str());

	strRes = m_oConfig.GetResponseString(11,103,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,103,-10250001)----------------\n%s\n\n",strRes.c_str());

	strRes = m_oConfig.GetResponseString(11,104,-10250001);
	BS_XLOG(XLOG_DEBUG, "\n\n");
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,104,-10250001)----------------\n%s\n\n",strRes.c_str());

	strRes = m_oConfig.GetResponseString(11,105,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,105,-10250001)----------------\n%s\n\n",strRes.c_str());

	strRes = m_oConfig.GetResponseString(11,106,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,106,-10250001)----------------\n%s\n\n",strRes.c_str());

	strRes = m_oConfig.GetResponseString(11,107,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,107,-10250001)----------------\n%s\n\n",strRes.c_str());

	strRes = m_oConfig.GetResponseString(11,108,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,108,-10250001)----------------\n%s\n\n",strRes.c_str());
	
	strRes = m_oConfig.GetResponseString(11,109,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,109,-10250001)----------------\n%s\n\n",strRes.c_str());
	
	strRes = m_oConfig.GetResponseString(11,110,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,110,-10250001)----------------\n%s\n\n",strRes.c_str());
	
	strRes = m_oConfig.GetResponseString(11,1112,-10250001);
	BS_XLOG(XLOG_DEBUG, "----------GetResponseString(11,1112,-10250001)----------------\n%s\n\n",strRes.c_str());
}
BOOST_AUTO_TEST_SUITE_END()




