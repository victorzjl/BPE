#ifndef _SDO_CIPHER_H_
#define _SDO_CIPHER_H_
namespace sdo{
namespace dbp{
class CCipher
{
public:
	static void CreateRandBytes(unsigned char* pbyBuff,int nLen);
	static void AesEnc(unsigned char abyKey[16],const unsigned char *pbyInBlk, int nInLen, unsigned char *pbyOutBlk);
	static void AesDec(unsigned char abyKey[16],const unsigned char *pbyInBlk, int nInLen, unsigned char *pbyOutBlk);
	static void Md5(const unsigned char *pbyInBlk, int nInLen, unsigned char *pbyOutBlk);
	
	static int Base64Encode(char * base64code, const char * src, int src_len=0) ;
	static int Base64Decode(char * buf, const char * base64code, int src_len=0) ;
	static int RsaPublicEncrypt(char *inbuf, int inlen, char *outbuf, int &outlen);
	static int RsaPrivateDecrypt(char *inbuf, int inlen, char *outbuf, int &outlen);
};
}
}
#endif
