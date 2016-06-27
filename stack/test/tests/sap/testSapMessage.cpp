#include <boost/test/unit_test.hpp>
#include "SapMessage.h"
#include <string>
using std::string;
using namespace sdo::sap;

BOOST_AUTO_TEST_SUITE(test_sap_message)
BOOST_AUTO_TEST_CASE(test_normal_message)
{
    CSapEncoder msg;
    msg.SetSequence(1);

    char szBuf[]="hello, world!";
	msg.SetValue(1,szBuf,7);
	msg.SetValue(2,10);
	msg.SetValue(3,"hello");

    CSapDecoder msgDecode(msg.GetBuffer(),msg.GetLen());
    msgDecode.DecodeBodyAsTLV();
    
    unsigned int nData=0;
    BOOST_CHECK_EQUAL(msgDecode.GetValue(2,&nData),0);
    BOOST_CHECK_EQUAL(nData,10);

    string strValue;
    BOOST_CHECK_EQUAL(msgDecode.GetValue(3,strValue),0);
    BOOST_CHECK_EQUAL(strValue,"hello");

    void *pData;
    unsigned int nLen;
    BOOST_CHECK_EQUAL(msgDecode.GetValue(1,&pData,&nLen),0);
    BOOST_CHECK_EQUAL(nLen,7);
    BOOST_CHECK_EQUAL(string((char *)pData,7),"hello, ");

}
BOOST_AUTO_TEST_CASE(test_enc_message)
{
    CSapEncoder msg;
    msg.SetSequence(1);

    char szBuf[]="hello, world!";
	msg.SetValue(1,szBuf,7);
    msg.SetValue(1,szBuf,8);
    
	msg.SetValue(2,10);
    msg.SetValue(2,20);
    
	msg.SetValue(3,"hello");
    msg.SetValue(3,"world");

    unsigned char *pKey=(unsigned char*)"1234567890123456";
    msg.AesEnc(pKey);

    CSapDecoder msgDecode(msg.GetBuffer(),msg.GetLen());
    msgDecode.AesDec(pKey);
	msgDecode.DecodeBodyAsTLV();
    
    unsigned int nData=0;
    BOOST_CHECK_EQUAL(msgDecode.GetValue(2,&nData),0);
    BOOST_CHECK_EQUAL(nData,10);

    string strValue;
    BOOST_CHECK_EQUAL(msgDecode.GetValue(3,strValue),0);
    BOOST_CHECK_EQUAL(strValue,"hello");

    void *pData;
    unsigned int nLen;
    BOOST_CHECK_EQUAL(msgDecode.GetValue(1,&pData,&nLen),0);
    BOOST_CHECK_EQUAL(nLen,7);
    BOOST_CHECK_EQUAL(string((char *)pData,7),"hello, ");

    vector<SSapValueNode> vecValueVoid;
    vector<unsigned int> vecValueInt;
    vector<string> vecValueStr;
    msgDecode.GetValues(1,vecValueVoid);
	msgDecode.GetValues(2,vecValueInt);
	msgDecode.GetValues(3,vecValueStr);

    vector<SSapValueNode>::iterator itr;
    itr=vecValueVoid.begin();
    SSapValueNode &node=*itr;
    BOOST_CHECK_EQUAL(node.nLen,7);
    BOOST_CHECK_EQUAL(string((char *)(node.pLoc),7),"hello, ");
    itr++;
    node=*itr;
    BOOST_CHECK_EQUAL(node.nLen,8);
    BOOST_CHECK_EQUAL(string((char *)(node.pLoc),8),"hello, w");

    vector<unsigned int>::iterator itr2=vecValueInt.begin();
    BOOST_CHECK_EQUAL(*itr2,10);
    itr2++;
    BOOST_CHECK_EQUAL(*itr2,20);

    vector<string>::iterator itr3=vecValueStr.begin();
    BOOST_CHECK_EQUAL(*itr3,"hello");
    itr3++;
    BOOST_CHECK_EQUAL(*itr3,"world");
}

BOOST_AUTO_TEST_SUITE_END()

