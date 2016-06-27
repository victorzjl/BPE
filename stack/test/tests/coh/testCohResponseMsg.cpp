#include <boost/test/unit_test.hpp>
#include "CohResponseMsg.h"

using namespace sdo::coh;

BOOST_AUTO_TEST_SUITE(test_cohresponsemsg)
BOOST_AUTO_TEST_CASE(test_parser)
{
    CCohResponseMsg msg;
    msg.SetStatus(0,"OK");

    msg.SetAttribute("test1", "value1");
    msg.SetAttribute("test2", 20);
    msg.SetAttribute("test3","aa");
    msg.SetAttribute("test3","bb");

    msg.Decode(msg.Encode());
    BOOST_CHECK_EQUAL(msg.GetStatusCode(),0);
    BOOST_CHECK_EQUAL(msg.GetStatusMsg(),"OK");
    BOOST_CHECK_EQUAL(msg.GetAttribute("test1"),"value1");
    BOOST_CHECK_EQUAL(msg.GetAttribute("test2"),"20");
    
    vector<string> vecCoh=msg.GetAttributes("test3");
    vector<string>::iterator itr=vecCoh.begin();
    string strV1=*itr++;
    string strV2=*itr++;

    BOOST_CHECK_EQUAL(strV1,string("aa"));
    BOOST_CHECK(strV2=="bb");
    
}
BOOST_AUTO_TEST_SUITE_END()

