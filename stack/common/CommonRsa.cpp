#include "CommonRsa.h"

namespace sdo{
namespace common{
  
		
CCommonRsa::CCommonRsa()
{
	
}

CCommonRsa::~CCommonRsa()
{
}

//公钥，模，私钥
bool CCommonRsa::SetEND(char *pPublicKey, char* pModule,  char *pPrivateKey)
{
	m_padding = RSA_NO_PADDING;
	m_bne = BN_new();
    m_bnd = BN_new();
    m_bnn = BN_new();
	unsigned int ne = atoi(pPublicKey);
	BN_set_word(m_bne, ne); //公钥
	BN_hex2bn(&m_bnd, pPrivateKey); //私钥
    BN_hex2bn(&m_bnn, pModule); //模
	
	m_r = RSA_new();
    m_r->e=m_bne;
    m_r->d=m_bnd;
    m_r->n=m_bnn;
	
	return true;
}

bool CCommonRsa::Encode(const unsigned char* iBuffer,unsigned char* oBuffer,int *nOutLen)
{
	int flen =  RSA_size(m_r);
	
	int ret =  RSA_public_encrypt(flen, iBuffer, oBuffer, m_r, m_padding);
	if (ret <0)
		return false;
	else 
	{
		*nOutLen = flen;
		return true;
	}
}

bool CCommonRsa::Decode(const unsigned char* iBuffer,unsigned char* oBuffer,int *nOutLen)
{
	int flen =  RSA_size(m_r);
	
	int ret =  RSA_private_decrypt(flen, iBuffer, oBuffer, m_r, m_padding);
	if (ret <0)
		return false;
	else 
	{
		*nOutLen = flen;
		return true;
	}
}

}
}


