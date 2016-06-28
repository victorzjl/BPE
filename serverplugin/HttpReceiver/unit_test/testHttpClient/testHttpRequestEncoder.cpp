#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp> 
#include <boost/test/unit_test_suite.hpp>
#include "HttpRequestEncoder.h"
#include "HttpClientLogHelper.h"

#include <iostream>
using namespace std;

struct MsgHttpRequestEncoderFixture
{
    MsgHttpRequestEncoderFixture(){cout << "MsgHttpRequestEncoderFixture()" << endl;}
    ~MsgHttpRequestEncoderFixture(){cout << "~MsgHttpRequestEncoderFixture()" << endl;}
};


//BOOST_AUTO_TEST_SUITE(test_requestMsgDecoder)
BOOST_FIXTURE_TEST_SUITE(test_httpRequestEncoder,MsgHttpRequestEncoderFixture)

// no body
BOOST_AUTO_TEST_CASE(test_httpRequestEncoder1)
{
	CHttpRequestEncoder encoder;
	string str = encoder.Encode("http://zhr.php5-3.com:8008/smgHttpServer/fail.php","","zhr.php5-3.com:8008");
	printf("\nstr:\n%s\n\n",str.c_str());
	

}


BOOST_AUTO_TEST_SUITE_END()



