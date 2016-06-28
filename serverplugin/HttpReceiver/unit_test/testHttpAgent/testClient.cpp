#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp> 
#include <boost/test/unit_test_suite.hpp>
#include <iostream> 
#include <boost/asio.hpp> 
#include <sys/time.h>
#include <iostream>
#include "HpsStack.h"
#include "HpsServer.h"

using namespace boost::asio; 
using namespace std;
using namespace sdo::hps;
/*

struct MsgClientFixture
{
    MsgClientFixture()
	{
		cout << "MsgClientFixture()" << endl;
		strIp = "192.168.90.22";
		nPort = 9997;

		CHpsStack::Instance()->Start(3);
		
		CHpsServer *pServer=new CHpsServer(CHpsStack::Instance()->GetThread()->GetIoService());
		
		if(pServer->StartServer(nPort)!=0)
		{
			fprintf(stderr, "run(), Hps server start fail,port[%d]!\n",nPort);
			return;
		} 			
	}
	
    ~MsgClientFixture()
	{
		cout << "~MsgClientFixture()" << endl;
	}

	io_service iosev; 
	int nPort;
	string strIp;
};


//BOOST_AUTO_TEST_SUITE(test_requestMsgDecoder)
BOOST_FIXTURE_TEST_SUITE(test_clientr,MsgClientFixture)

//正常数据包
BOOST_AUTO_TEST_CASE(test_Get1)
{
	ip::tcp::socket socket(iosev);
	socket.open(boost::asio::ip::tcp::v4());

	boost::system::error_code ec; 
	ip::tcp::endpoint epServer(ip::address_v4::from_string(strIp), nPort);  //server port
	socket.connect(epServer,ec); 
	if(ec) 
	{ 
		std::cout << "connect server error: " <<  boost::system::system_error(ec).what() << std::endl; 
	} 

	string strRequest = "GET /ECard/QueryInfo.fcgi?a=1&b=2&c=3 HTTP/1.1\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\r\n\
Accept-Charset:GB2312,utf-8;q=0.7,*;q=0.7\r\n\
Keep-Alive:115\r\n\
Connection:keep-alive\r\n\r\n";

	socket.send(boost::asio::buffer(strRequest, strRequest.size()));

	char buf[1024];
	int size = socket.receive(boost::asio::buffer(buf, 1024));

	buf[size] = '\0';
	printf("------ %d -----\n", __LINE__);
	printf("------ %s\n", buf);
	printf("------ %d -----\n", __LINE__);
		
	socket.close();
}


BOOST_AUTO_TEST_SUITE_END()

*/

