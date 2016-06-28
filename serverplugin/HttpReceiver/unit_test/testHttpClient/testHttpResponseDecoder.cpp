#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp> 
#include <boost/test/unit_test_suite.hpp>
#include "HttpResponseDecoder.h"
#include "HttpClientLogHelper.h"

#include <iostream>
using namespace std;

struct MsgHttpResponseDecoderFixture
{
    MsgHttpResponseDecoderFixture(){cout << "MsgHttpResponseDecoderFixture()" << endl;}
    ~MsgHttpResponseDecoderFixture(){cout << "~MsgHttpResponseDecoderFixture()" << endl;}
};


//BOOST_AUTO_TEST_SUITE(test_requestMsgDecoder)
BOOST_FIXTURE_TEST_SUITE(test_httpResonseDecoder,MsgHttpResponseDecoderFixture)

// no body
BOOST_AUTO_TEST_CASE(test_httpResonseDecoder1)
{

	char *pResponse = "HTTP/1.1 200 OK\r\n\
Date	Sun, 21 Aug 2011 12:13:30 GMT\r\n\
Server	Apache/2.2.8 (Win32) PHP/5.2.5\r\n\
X-Powered-By	PHP/5.2.5\r\n\
Expires	Thu, 19 Nov 1981 08:52:00 GMT\r\n\
Cache-Control	no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n\
Pragma	no-cache\r\n\
Content-Length	6623\r\n\
Keep-Alive	timeout=5, max=99\r\n\
Connection	Keep-Alive\r\n\
Content-Type	text/html\r\n\
\r\n";

	CHttpResponseDecoder decoder;
	decoder.Decode(pResponse, strlen(pResponse));
	
	BOOST_CHECK_EQUAL(decoder.GetHttpCode(),200);

}



BOOST_AUTO_TEST_CASE(test_httpResonseDecoder2)
{
	char *pResponse = "HTTP/1.1 200 OK\r\n\
Server: nginx/0.8.54\r\n\
Date: Mon, 24 Oct 2011 09:41:12 GMT\r\n\
Content-Type: text/html\r\n\
Transfer-Encoding: chunked\r\n\
Connection: keep-alive\r\n\
X-Powered-By: PHP/5.2.17\r\n\
\r\n";

	CHttpResponseDecoder decoder;
	decoder.Decode(pResponse, strlen(pResponse));
	
	BOOST_CHECK_EQUAL(decoder.GetHttpCode(),200);

	decoder.Dump();

}


BOOST_AUTO_TEST_CASE(test_httpResonseDecoder3)
{
	char *pResponse = "HTTP/1.1 200 OK\r\n\
Server: nginx/0.8.54\r\n\
Date: Mon, 24 Oct 2011 09:41:12 GMT\r\n\
Content-Type: text/html\r\n\
Transfer-Encoding:  chunked\r\n\
Connection:    keep-alive\r\n\
X-Powered-By: PHP/5.2.17\r\n\
\r\n\
A\r\n\
1234567890\r\n\
1E\r\n\
abcdefghijklmnopqrstuvwxyz\r\n12\r\n\
0\r\n";

	CHttpResponseDecoder decoder;
	decoder.Decode(pResponse, strlen(pResponse));
	
	BOOST_CHECK_EQUAL(decoder.GetHttpCode(),200);

	decoder.Dump();

}

BOOST_AUTO_TEST_CASE(test_httpResonseDecoder4)
{
	char *pResponse = "HTTP/1.1 200 OK\r\n\
  Server  :  nginx/0.8.54  \r\n\
    Date       : Mon, 24 Oct 2011 09:41:12 GMT         \r\n\
Content-Type: text/html\r\n\
 Transfer-Encoding :  chunked \r\n\
Connection:    keep-alive\r\n\
X-Powered-By: PHP/5.2.17\r\n\
\r\n\
A\r\n\
1234567890\r\n\
1E\r\n\
abcdefghijklmnopqrstuvwxyz\r\n12\r\n\
0\r\n";

	CHttpResponseDecoder decoder;
	decoder.Decode(pResponse, strlen(pResponse));
	
	BOOST_CHECK_EQUAL(decoder.GetHttpCode(),200);

	decoder.Dump();

}

BOOST_AUTO_TEST_CASE(test_httpResonseDecoder5)
{
	char *pResponse = "HTTP/1.1 200 OK\r\n\
  Server  :  nginx/0.8.54  \r\n\
    Date       : Mon, 24 Oct 2011 09:41:12 GMT         \r\n\
Content-Type: text/html\r\n\
 Transfer-Encoding :  chunked \r\n\
Connection:    keep-alive\r\n\
X-Powered-By: PHP/5.2.17\r\n\
\r\n\
A\r\n\
1234567890\r\n\
1E\r\n\
abcdefghijklmnopqrstuvwxyz\r\n12\r\n\
0\r\n";

	CHttpResponseDecoder decoder;
	decoder.Decode(pResponse, strlen(pResponse)+100);
	
	BOOST_CHECK_EQUAL(decoder.GetHttpCode(),200);

	decoder.Dump();

}

BOOST_AUTO_TEST_CASE(test_httpResonseDecoder6)
{
	char *pResponse = "HTTP/1.1 200 OK\r\n\
  Server  :  nginx/0.8.54  \r\n\
    Date       : Mon, 24 Oct 2011 09:41:12 GMT         \r\n\
Content-Type: text/html\r\n\
 Transfer-Encoding :  chunked \r\n\
Connection:    keep-alive\r\n\
X-Powered-By: PHP/5.2.17\r\n\
\r\n\
A\r\n\
1234567890\r\n\
1E\r\n\
abcdefghijklmnopqrstuvwxyz12\r\n\
0\r\n";

	CHttpResponseDecoder decoder;
	decoder.Decode(pResponse, strlen(pResponse)-50);
	
	BOOST_CHECK_EQUAL(decoder.GetHttpCode(),200);

	decoder.Dump();

}

BOOST_AUTO_TEST_CASE(test_httpResonseDecoder7)
{
	char *pResponse = "HTTP/1.1 200 OK\r\n\
  Server  :  nginx/0.8.54  \r\n\
    Date       : Mon, 24 Oct 2011 09:41:12 GMT         \r\n\
Content-Type: text/html\r\n\
 Transfer-Encoding :  chunked \r\n\
Connection:    keep-alive\r\n\
X-Powered-By: PHP/5.2.17\r\n\
\r\n\
A\r\n\
1234567890\r\n\
AA\r\n\
111111111abcdefghijklmnopqrstuvwxyz12\r\n\
0\r\n";

	CHttpResponseDecoder decoder;
	int nRet = decoder.Decode(pResponse, strlen(pResponse)+200);
	
	BOOST_CHECK_EQUAL(decoder.GetHttpCode(),200);
	//BOOST_CHECK_EQUAL(nRet,0);

	decoder.Dump();

}

BOOST_AUTO_TEST_CASE(test_httpResonseDecoder8)
{
	char *pResponse = "HTTP/1.1 200 OK\r\n\
Server:  nginx/0.8.54  \r\n\
Date: Mon, 24 Oct 2011 09:41:12 GMT         \r\n\
Content-Length: 20\r\n\
Connection:    keep-alive\r\n\
X-Powered-By: PHP/5.2.17\r\n\
\r\n\
12345678901234567890";

	CHttpResponseDecoder decoder;
	int nRet = decoder.Decode(pResponse, strlen(pResponse));
	
	BOOST_CHECK_EQUAL(decoder.GetHttpCode(),200);
	BOOST_CHECK_EQUAL(nRet,0);

	decoder.Dump();

}

BOOST_AUTO_TEST_CASE(test_httpResonseDecoder9)
{
	char *pResponse = "HTTP/1.1 200 OK\r\n\
Server:  nginx/0.8.54  \r\n\
Date: Mon, 24 Oct 2011 09:41:12 GMT         \r\n\
Connection:    keep-alive\r\n\
X-Powered-By: PHP/5.2.17\r\n\
\r\n\
12345678901234567890\r\n";

	CHttpResponseDecoder decoder;
	int nRet = decoder.Decode(pResponse, strlen(pResponse));
	
	BOOST_CHECK_EQUAL(decoder.GetHttpCode(),200);
	BOOST_CHECK_EQUAL(nRet,0);

	HTTPCLIENT_XLOG(XLOG_DEBUG,"----body[%s]\n",decoder.GetBody().c_str());
	decoder.Dump();
}



BOOST_AUTO_TEST_SUITE_END()


