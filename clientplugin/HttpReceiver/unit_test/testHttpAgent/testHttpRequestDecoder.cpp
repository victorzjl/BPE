#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp> 
#include <boost/test/unit_test_suite.hpp>
#include "HpsRequestMsg.h"
#include <iostream>
using namespace std;
using namespace sdo::hps;
/*
struct MsgRequestMsgFixture
{
    MsgRequestMsgFixture(){cout << "MsgRequestMsgFixture()" << endl;}
    ~MsgRequestMsgFixture(){cout << "~MsgRequestMsgFixture()" << endl;}
};


//BOOST_AUTO_TEST_SUITE(test_requestMsgDecoder)
BOOST_FIXTURE_TEST_SUITE(test_requestMsgDecoder,MsgRequestMsgFixture)

//正常数据包
BOOST_AUTO_TEST_CASE(test_Get1)
{
	char *pGetPacket = "GET /ECard/QueryInfo.fcgi?a=1&b=2&c=3 HTTP/1.1\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\r\n\
Accept-Charset:GB2312,utf-8;q=0.7,*;q=0.7\r\n\
Keep-Alive:115\r\n\
Connection:keep-alive\r\n\r\n";

	int nLen = strlen(pGetPacket);
	char szBuf[10240] = {0};
	memcpy(szBuf,pGetPacket,nLen);
    CHpsRequestMsg requestDecoder;
	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"ECard/QueryInfo.fcgi");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),1);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"a=1&b=2&c=3");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),false);
}

//无HTTP版本号信息，默认为0
BOOST_AUTO_TEST_CASE(test_Get2)
{
	char *pGetPacket = "GET /ECard/QueryInfo.fcgi?a=1&b=2&c=3HTTP/1.0\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\r\n\
Accept-Char\r\n\
Keep-Alive:115\r\n\
Connection:Closen\r\n\r\n";

	int nLen = strlen(pGetPacket);
	char szBuf[10240] = {0};
	memcpy(szBuf,pGetPacket,nLen);
    CHpsRequestMsg requestDecoder;
	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"ECard/QueryInfo.fcgi");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),0);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),false);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"a=1&b=2&c=3HTTP/1.0");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),false);
}

//非法结束符,无attribute
BOOST_AUTO_TEST_CASE(test_Get3)
{
	char *pGetPacket = "GET /ECard/QueryInfo.fcgi HTTP/1.1\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\rn\
Accept-Encoding:gzip,deflate\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
Connection:Close\r\n\
Accept-Char\r\n\r\n";

	int nLen = strlen(pGetPacket);
	char szBuf[10240] = {0};
	memcpy(szBuf,pGetPacket,nLen);
    CHpsRequestMsg requestDecoder;
	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"ECard/QueryInfo.fcgi");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),1);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),false);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetXForwardedFor(),"19.19.12.12");
}

//http1.1 默认is keep-alive 为true
BOOST_AUTO_TEST_CASE(test_Get4)
{
	char *pGetPacket = "GET /ECard/QueryInfo.fcgi HTTP/1.1\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\r\n";

	int nLen = strlen(pGetPacket);
	char szBuf[10240] = {0};
	memcpy(szBuf,pGetPacket,nLen);
    CHpsRequestMsg requestDecoder;
	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"ECard/QueryInfo.fcgi");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),1);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetXForwardedFor(),"19.19.12.12");
}

//http1.0 默认is keep-alive 为false
BOOST_AUTO_TEST_CASE(test_Get5)
{
	char *pGetPacket = "GET /ECard/QueryInfo.fcgi HTTP/1.0\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\r\n";

	int nLen = strlen(pGetPacket);
	char szBuf[10240] = {0};
	memcpy(szBuf,pGetPacket,nLen);
    CHpsRequestMsg requestDecoder;
	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"ECard/QueryInfo.fcgi");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),0);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),false);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetXForwardedFor(),"19.19.12.12");
}

//GET非法头部
BOOST_AUTO_TEST_CASE(test_Get6)
{
	char *pGetPacket = "GET HTTP/1.0\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9*;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\r\n";

	int nLen = strlen(pGetPacket);
	char szBuf[10240] = {0};
	memcpy(szBuf,pGetPacket,nLen);
    CHpsRequestMsg requestDecoder;
	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),-1);
}

//GET非法头部
BOOST_AUTO_TEST_CASE(test_Get7)
{
	char *pGetPacket = "GET123\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\r\n";

	int nLen = strlen(pGetPacket);
	char szBuf[10240] = {0};
	memcpy(szBuf,pGetPacket,nLen);
    CHpsRequestMsg requestDecoder;
	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),-1);
}

//POST方法正常数据
BOOST_AUTO_TEST_CASE(test_Post1)
{
	char *pPostHead = "POST command HTTP/1.0\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
Content-Length:9\r\n\r\n\
post data";

	char *pBody = "post data";
	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"command");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),0);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),false);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"post data");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetXForwardedFor(),"19.19.12.12");
}

//POST方法正常数据
BOOST_AUTO_TEST_CASE(test_Post2)
{
	char *pPostHead = "POST command HTTP/1.1\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
Content-Length:9\r\n\r\n\
post data";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"command");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),1);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"post data");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetXForwardedFor(),"19.19.12.12");
}

//POST方法正常数据 connection:close
BOOST_AUTO_TEST_CASE(test_Post3)
{
	char *pPostHead = "POST command HTTP/1.1\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
connection:close\r\n\
Content-Length:9\r\n\r\n\
post data";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"command");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),1);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),false);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"post data");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetXForwardedFor(),"19.19.12.12");
}

//POST方法正常数据
BOOST_AUTO_TEST_CASE(test_Post4)
{
	char *pPostHead = "POST command HTTP/1.1\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
connection:keep-alive\r\n\
Content-Length:9\r\n\r\n\
post data";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"command");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),1);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"post data");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetXForwardedFor(),"19.19.12.12");
}

//POST方法正常数据 http1.0 connecntion:keep-alive
BOOST_AUTO_TEST_CASE(test_Post5)
{
	char *pPostHead = "POST command HTTP/1.0\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
connection:keep-alive\r\n\
Content-Length:9\r\n\r\n\
post data";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"command");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),0);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"post data");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetXForwardedFor(),"19.19.12.12");
}

//POST方法正常数据 http1.0 connecntion:close
BOOST_AUTO_TEST_CASE(test_Post6)
{
	char *pPostHead = "POST command HTTP/1.0\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
connection:close\r\n\
Content-Length:9\r\n\r\n\
post data";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),0);
	BOOST_CHECK_EQUAL(requestDecoder.GetCommand(),"command");
	BOOST_CHECK_EQUAL(requestDecoder.GetHttpVersion(),0);
	BOOST_CHECK_EQUAL(requestDecoder.IsKeepAlive(),false);
	BOOST_CHECK_EQUAL(requestDecoder.GetBody(),"post data");
	BOOST_CHECK_EQUAL(requestDecoder.IsXForwardedFor(),true);
	BOOST_CHECK_EQUAL(requestDecoder.GetXForwardedFor(),"19.19.12.12");
}

//POST方法正常数据 http1.0 content-length过短
BOOST_AUTO_TEST_CASE(test_Post7)
{
	char *pPostHead = "POST command HTTP/1.0\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
connection:close\r\n\
Content-Length:20\r\n\r\n\
123456789012345678901234567890";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),-1);
}

//POST方法正常数据 http1.0 content-length超长
BOOST_AUTO_TEST_CASE(test_Post8)
{
	char *pPostHead = "POST command HTTP/1.0\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
connection:close\r\n\
Content-Length:40\r\n\r\n\
123456789012345678901234567890";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),-1);
}

//POST illegale packet
BOOST_AUTO_TEST_CASE(test_Post9)
{
	char *pPostHead = "POST commandHTTP/1.0\
Host:192.168.90.22:10091\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\
Accept-Language:zh-cn,zh;q=0.5\
Accept-Encoding:gzip,deflate\
Accept-Char\
x-forwarded-for:19.19.12.12\
Keep-Alive:115\
Content-Length:9\
post data";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),-1);
}


//POST 
BOOST_AUTO_TEST_CASE(test_Post10)
{
	char *pPostHead = "POST commandHTTP/1.0\r\n\
Host:192.168.90.22:10091\r\n\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\r\n\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\r\n\
Accept-Language:zh-cn,zh;q=0.5\r\n\
Accept-Encoding:gzip,deflate\
Accept-Char\r\n\
x-forwarded-for:19.19.12.12\r\n\
Keep-Alive:115\r\n\
Content-Length:9\r\n\r\n\
post data";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),-1);
}

//POST illegale packet
BOOST_AUTO_TEST_CASE(test_Post11)
{
	char *pPostHead = "POST command/hello HTTP/1.0\
Host:192.168.90.22:10091\
User-Agent:Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.15) Gecko/20110303 Firefox/3.6.15 (.NET CLR 3.5.30729)\
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8\
Accept-Language:zh-cn,zh;q=0.5\
Accept-Encoding:gzip,deflate\
Accept-Char\
x-forwarded-for:19.19.12.12\
Keep-Alive:115\
Content-Length:9\
post data";

	char szBuf[10240] = {0};
	//sprintf(szBuf,"%sContent-Length:%d\r\n\r\n%s",pPostHead,strlen(pBody),pBody);
	int nLen = strlen(pPostHead);
	memcpy(szBuf,pPostHead,nLen);
	
    CHpsRequestMsg requestDecoder;	
    BOOST_CHECK_EQUAL(requestDecoder.Decode(szBuf,nLen),-1);

}


BOOST_AUTO_TEST_SUITE_END()
*/

