#include <boost/test/unit_test.hpp>
#include "Cipher.h"
#include <string>
#include <cstdio>
using std::string;
using namespace sdo::common;

static char *pt(unsigned char *md)	
{   
    int i;
    static char buf[80];
    for (i=0; i<16; i++)
        sprintf(&(buf[i*2]),"%02x",md[i]);
    return(buf);
}


BOOST_AUTO_TEST_SUITE(test_cipher)

BOOST_AUTO_TEST_CASE(test_rand)
{
    unsigned char szRand1[16];
	unsigned char szRand2[16];
	memset(szRand1,0,16);
	memset(szRand2,0,16);
	
	BOOST_CHECK(string((char *)szRand1,16)==string((char *)szRand2,16));
	
	CCipher::CreateRandBytes(szRand1,16);
	CCipher::CreateRandBytes(szRand2,16);
	
    BOOST_CHECK(string((char *)szRand1,16)!=string((char *)szRand2,16));
}

BOOST_AUTO_TEST_CASE(test_aes)
{
	unsigned char szSrc[32];
	unsigned char szKey[16];
	unsigned char szResult[32];
    unsigned char szSrcResult[32];
	unsigned char szNewSrc[32];
	CCipher::CreateRandBytes(szSrc,32);
	CCipher::CreateRandBytes(szKey,16);
	
	CCipher::AesEnc(szKey,szSrc,32,szResult);
	BOOST_CHECK(string((char *)szSrc,32)!=string((char *)szResult,32));

    memcpy(szSrcResult,szSrc,32);
    CCipher::AesEnc(szKey,szSrcResult,32,szSrcResult);
    BOOST_CHECK(string((char *)szResult,32)==string((char *)szSrcResult,32));
    
	
	CCipher::AesDec(szKey,szResult,32,szNewSrc);
	BOOST_CHECK(string((char *)szSrc,32)==string((char *)szNewSrc,32));

    CCipher::AesDec(szKey,szResult,32,szResult);
    BOOST_CHECK(string((char *)szSrc,32)==string((char *)szResult,32));
}
BOOST_AUTO_TEST_CASE(test_md5)
{
    char *test[]={
        "", 
        "a",
        "abc",
        "message digest",
        "abcdefghijklmnopqrstuvwxyz",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
        "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
        NULL,
        };
    char *ret[]={
        "d41d8cd98f00b204e9800998ecf8427e",
        "0cc175b9c0f1b6a831c399e269772661",
        "900150983cd24fb0d6963f7d28e17f72",
        "f96b697d7cb7938d525a2f31aaf161d0",
        "c3fcd3d76192e4007dfb496cca67e13b",
        "d174ab98d277d9f5a5611c2c9f419d9f",
        "57edf4a22be3c955ac49da2e2107b67a",
        };

    char **P,**R;
    char *p;
    unsigned char md[16];
    P=test;
    R=ret;
    while (*P != NULL)
    {
        CCipher::Md5((unsigned char *)(&(P[0][0])),strlen((char *)*P),md);
        p=pt(md);
        BOOST_CHECK(strcmp(p,(char *)*R) == 0);
        P++;R++;
    }
}

BOOST_AUTO_TEST_SUITE_END()

