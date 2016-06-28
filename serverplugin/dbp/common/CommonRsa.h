#ifndef _COMMON_RSA_H_
#define _COMMON_RSA_H_

#include "openssl/rsa.h"
namespace sdo{
namespace dbp{

class CCommonRsa
{
public:
	static const char* HEXCHAR;
	CCommonRsa();
	~CCommonRsa();
	bool SetEND(char *pPublicKey, char *pModule,  char *pPrivateKey);
	
	bool Encode(const unsigned char* iBuffer,unsigned char* oBuffer,int *nOutLen);
	bool Decode(const unsigned char* iBuffer,unsigned char* oBuffer,int *nOutLen);
	
private:
	RSA *m_r;
	BIGNUM *m_bne,*m_bnn,*m_bnd;
	int m_padding;
};
}
}
#endif







