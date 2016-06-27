#include <boost/test/unit_test.hpp>
#include "XmlConfigParser.h"
#include <cstdio>
using namespace sdo::common;

BOOST_AUTO_TEST_SUITE(test_xmlconfig)
BOOST_AUTO_TEST_CASE(test_read)
{
    const char *p="<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" \
        "<parameters>" \
	        "<sapport>4042</sapport>" \
	        "<adminuser>admin</adminuser>" \
	        "<adminpass><test>hello</test></adminpass>" \
	        "<cohport>9092</cohport>" \
	        "<cohport>9093</cohport>" \
        "</parameters>";
    CXmlConfigParser oConfig;
    BOOST_CHECK(oConfig.ParseBuffer(p)==0);

    BOOST_CHECK_EQUAL(oConfig.GetParameter("SapPort",2),4042);
    BOOST_CHECK(oConfig.GetParameter("SapPort","2")==string("4042"));

    BOOST_CHECK(oConfig.GetParameter("AdminUser","2")==string("admin"));
    BOOST_CHECK(oConfig.GetParameter("AdminPass/test","2")==string("hello"));

    BOOST_CHECK(oConfig.GetParameter("None","2")==string("2"));
    BOOST_CHECK(oConfig.GetParameter("None",2)==2);

    vector<string> vecCoh=oConfig.GetParameters("cohport");
    vector<string>::iterator itr=vecCoh.begin();
    string strPort1=*itr++;
    string strPort2=*itr++;

    BOOST_CHECK_EQUAL(strPort1,string("9092"));
    BOOST_CHECK(strPort2=="9093");
}
BOOST_AUTO_TEST_SUITE_END()

