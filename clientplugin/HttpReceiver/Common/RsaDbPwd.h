#ifndef _RSA_DB_PWD_H_
#define _RSA_DB_PWD_H_

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include "CommonRsa.h"
#include "Cipher.h"
#include "BusinessLogHelper.h"

using namespace sdo::common;

#define RSA_MODULE "00bf63d4b708b67e3926e97272ffafdf715f5fdf5f46bfa7d5e4617685c73a31c2d20cf76be6fef1d8466fedc60d93ac757adedc77800d946444a56c4b20f2a010d446ec812701a234337377a73da04a60f56986944aa380d2784b07c52fe67be45c3d1fc4a3adeeb0a994ec1e7944e9c1f5d92447f09736417362d0ae2ca4128b"
#define RSA_PUBLIC 65537
#define RSA_PRIVATE "6ea7867dc8b0d26bb2beb8281b02913a983cf305bffee147be4247677357871baf9c4595023a1693c7adc1188813005bf00d5804536a0688ae53c237f1b5b5aae42ad34385d1ca6a7d7da1bb7deaab2b4eb76eff554dc015d1de0c9fc7c0f4405d91b472e7d06c1cb5e7fbb221b422ab747fbabad804ff33d4339a7cb7621811"


class CRsaBase64
{
public:
	static int Decode(char *szOutText, int *nOutLen, const char *szInText, int nInLen)
	{
		char szBase64Decode[1024] = {0};
		int nBase64DecodeLen = CCipher::Base64Decode(szBase64Decode,szInText,nInLen);
		if(nBase64DecodeLen<0)
		{
			BS_XLOG(XLOG_ERROR,"CRsaBase64::%s, Base64Decode fail[%s]\n",__FUNCTION__,szInText);	
			return -1;
		}


		
		BIGNUM *bnn= BN_new();
		BIGNUM *bne= BN_new();
		BIGNUM *bnd= BN_new();

	    BN_hex2bn(&bnn, RSA_MODULE);
	    BN_set_word(bne, RSA_PUBLIC);
	    BN_hex2bn(&bnd, RSA_PRIVATE);

	    RSA *rsa_private = RSA_new();
	    rsa_private->n = bnn;
	    rsa_private->e = bne;
	    rsa_private->d = bnd;

		*nOutLen = RSA_private_decrypt( nBase64DecodeLen, (unsigned char *)szBase64Decode, (unsigned char *)szOutText, rsa_private, RSA_PKCS1_PADDING );

		RSA_free( rsa_private );
		return 0;
	}

	static int ConvertDbPwd(const string &strEncBase64, string &strNew)
	{
		char szPwd[1024]={0};
		int nPwdLen = 0;
		CRsaBase64::Decode(szPwd,&nPwdLen,strEncBase64.c_str(),strEncBase64.size());
		if(nPwdLen<=0)
		{
			BS_XLOG(XLOG_ERROR,"CRsaBase64::%s, decode text fail[%s]\n",__FUNCTION__,strEncBase64.c_str());
			return -1;
		}

		strNew = string(szPwd,nPwdLen);
		return 0;
	}
};
#endif



