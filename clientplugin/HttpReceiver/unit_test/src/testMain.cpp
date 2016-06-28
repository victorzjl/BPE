#define BOOST_TEST_MODULE hpstest
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "HttpClientLogHelper.h"
#include "BusinessLogHelper.h"


using namespace std;

struct MyFixture
{
    MyFixture()
	{
		std::cout<<"MyFixture constructor\n";
		XLOG_INIT("./conf/log.properties", true);
		XLOG_REGISTER(HTTP_CLIENT_STACK_MODULE, "client_stack_module");
		XLOG_REGISTER(BUSINESS_MODULE, "business");
	}
    ~MyFixture()
	{
		std::cout<<"~MyFixture deconstructor\n";
    }
};
BOOST_GLOBAL_FIXTURE(MyFixture);


