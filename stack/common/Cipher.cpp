#include "Cipher.h"
#include <cstdio>
#include <openssl/md5.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <cstdlib>
#include <cstring>



#define B0_0(a) (a & 0xFF)
#define B1(a) (a >> 8 & 0xFF)
#define B2(a) (a >> 16 & 0xFF)
#define B3(a) (a >> 24 & 0xFF)

namespace sdo{
namespace common{

void CCipher::CreateRandBytes(unsigned char*pbyBuff,int nLen)
{
    RAND_pseudo_bytes(pbyBuff,nLen);
}
void CCipher::AesEnc(unsigned char abyKey[16],const unsigned char *pbyInBlk, int nInLen, unsigned char *pbyOutBlk)
{
    AES_KEY aes;
    unsigned char ivec[16]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x30,0x31,0x32,0x33,0x34,0x35};
    const unsigned char *iv = ivec;
    AES_set_encrypt_key(abyKey, 128, &aes);
    int len=((nInLen&0x0f)!=0?(((nInLen>>4)+1)<<4):nInLen);

    int n;
    if(pbyOutBlk!=pbyInBlk)
    {
        //AES_cbc_encrypt((unsigned char*)pbyInBlk, (unsigned char*)pbyOutBlk, len, &aes, (unsigned char*)iv,AES_ENCRYPT);
        while (len >= AES_BLOCK_SIZE) {
            for(n=0; n < AES_BLOCK_SIZE; ++n)
                pbyOutBlk[n] = pbyInBlk[n] ^ iv[n];
            AES_encrypt(pbyOutBlk, pbyOutBlk, &aes);
            iv = pbyOutBlk;
            len -= AES_BLOCK_SIZE;
            pbyInBlk += AES_BLOCK_SIZE;
            pbyOutBlk += AES_BLOCK_SIZE;
        }
        if (len) {
            for(n=0; n < len; ++n)
                pbyOutBlk[n] = pbyInBlk[n] ^ iv[n];
            for(n=len; n < AES_BLOCK_SIZE; ++n)
                pbyOutBlk[n] = iv[n];
            AES_encrypt(pbyOutBlk, pbyOutBlk, &aes);
            iv = pbyOutBlk;
        }
    }
    else
    {   
        unsigned char tmp[AES_BLOCK_SIZE];
        while (len >= AES_BLOCK_SIZE) {
            memcpy(tmp, pbyInBlk, AES_BLOCK_SIZE);
			for(n=0; n < AES_BLOCK_SIZE; ++n)
				pbyOutBlk[n] = tmp[n] ^ iv[n];
			AES_encrypt(pbyOutBlk, pbyOutBlk, &aes);
			iv = pbyOutBlk;
			len -= AES_BLOCK_SIZE;
			pbyInBlk += AES_BLOCK_SIZE;
			pbyOutBlk += AES_BLOCK_SIZE;
		}
		if (len) {
            memcpy(tmp, pbyInBlk, AES_BLOCK_SIZE);
			for(n=0; n < len; ++n)
				pbyOutBlk[n] = tmp[n] ^ iv[n];
			for(n=len; n < AES_BLOCK_SIZE; ++n)
				pbyOutBlk[n] = iv[n];
			AES_encrypt(pbyOutBlk, pbyOutBlk, &aes);
			iv = pbyOutBlk;
		}
    }
}
void CCipher::AesDec(unsigned char abyKey[16],const unsigned char *pbyInBlk, int nInLen, unsigned char *pbyOutBlk)
{
    AES_KEY aes;
    unsigned char ivec[16]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x30,0x31,0x32,0x33,0x34,0x35};
    unsigned char *iv = ivec;
    AES_set_decrypt_key((unsigned char*)abyKey, 128, &aes);
    int len=((nInLen&0x0f)!=0?((nInLen>>4)+1)<<4:nInLen);
    int n;
    unsigned char tmp[AES_BLOCK_SIZE];
    if(pbyInBlk==pbyOutBlk)
    {
        while (len >= AES_BLOCK_SIZE) {
            memcpy(tmp, pbyInBlk, AES_BLOCK_SIZE);
            AES_decrypt(pbyInBlk, pbyOutBlk, &aes);
            for(n=0; n < AES_BLOCK_SIZE; ++n)
                pbyOutBlk[n] ^= iv[n];
            memcpy(iv, tmp, AES_BLOCK_SIZE);
            len -= AES_BLOCK_SIZE;
            pbyInBlk += AES_BLOCK_SIZE;
            pbyOutBlk += AES_BLOCK_SIZE;
            }
        if (len) {
            memcpy(tmp, pbyInBlk, AES_BLOCK_SIZE);
            AES_decrypt(tmp, pbyOutBlk, &aes);
            for(n=0; n < len; ++n)
                pbyOutBlk[n] ^= iv[n];
            for(n=len; n < AES_BLOCK_SIZE; ++n)
                pbyOutBlk[n] = tmp[n];
            }
    }
    //AES_cbc_encrypt((unsigned char*)pbyInBlk, (unsigned char*)pbyOutBlk, len, &aes, (unsigned char*)iv,AES_DECRYPT);
    else
    {
        
        while (len >= AES_BLOCK_SIZE) {
            AES_decrypt(pbyInBlk, pbyOutBlk, &aes);
            for(n=0; n < AES_BLOCK_SIZE; ++n)
                pbyOutBlk[n] ^= iv[n];
            iv = (unsigned char*)pbyInBlk;
            len -= AES_BLOCK_SIZE;
            pbyInBlk  += AES_BLOCK_SIZE;
            pbyOutBlk += AES_BLOCK_SIZE;
            }
        if (len) {
            AES_decrypt(pbyInBlk,tmp,&aes);
            for(n=0; n < len; ++n)
                pbyOutBlk[n] = tmp[n] ^ iv[n];
            iv = (unsigned char*)pbyInBlk;
            } 
    }
}
void CCipher::Md5(const unsigned char *pbyInBlk, int nInLen, unsigned char *pbyOutBlk)
{
    MD5(pbyInBlk, nInLen ,pbyOutBlk);
}


char GetB64Char(int index)
{
	const char szBase64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (index >= 0 && index < 64)
		return szBase64Table[index];
	
	return '=';
}
int GetB64Index(char ch)
{
	int index = -1;
	if (ch >= 'A' && ch <= 'Z')
	{
		index = ch - 'A';
	}
	else if (ch >= 'a' && ch <= 'z')
	{
		index = ch - 'a' + 26;
	}
	else if (ch >= '0' && ch <= '9')
	{
		index = ch - '0' + 52;
	}
	else if (ch == '+')
	{
		index = 62;
	}
	else if (ch == '/')
	{
		index = 63;
	}

	return index;
}
int CCipher::Base64Encode(char * base64code, const char * src, int src_len) 
{   
	if (src_len == 0)
		src_len = strlen(src);
	
	int len = 0;
	unsigned char* psrc = (unsigned char*)src;
	char * p64 = base64code;
	int i;
	for (i = 0; i < src_len - 3; i += 3)
	{
		unsigned long ulTmp = *(unsigned long*)psrc;
		register int b0 = GetB64Char((B0_0(ulTmp) >> 2) & 0x3F); 
		register int b1 = GetB64Char((B0_0(ulTmp) << 6 >> 2 | B1(ulTmp) >> 4) & 0x3F); 
		register int b2 = GetB64Char((B1(ulTmp) << 4 >> 2 | B2(ulTmp) >> 6) & 0x3F); 
		register int b3 = GetB64Char((B2(ulTmp) << 2 >> 2) & 0x3F); 
		*((unsigned long*)p64) = b0 | b1 << 8 | b2 << 16 | b3 << 24;
		len += 4;
		p64  += 4; 
		psrc += 3;
	}
	
	// 处理最后余下的不足3字节的饿数据
	if (i < src_len)
	{
		int rest = src_len - i;
		unsigned long ulTmp = 0;
		for (int j = 0; j < rest; ++j)
		{
			*(((unsigned char*)&ulTmp) + j) = *psrc++;
		}
		
		p64[0] = GetB64Char((B0_0(ulTmp) >> 2) & 0x3F); 
		p64[1] = GetB64Char((B0_0(ulTmp) << 6 >> 2 | B1(ulTmp) >> 4) & 0x3F); 
		p64[2] = rest > 1 ? GetB64Char((B1(ulTmp) << 4 >> 2 | B2(ulTmp) >> 6) & 0x3F) : '='; 
		p64[3] = rest > 2 ? GetB64Char((B2(ulTmp) << 2 >> 2) & 0x3F) : '='; 
		p64 += 4; 
		len += 4;
	}
	
	*p64 = '\0'; 
	
	return len;
}
// 解码后的长度一般比原文少用占1/4的存储空间，请保证buf有足够的空间
int CCipher::Base64Decode(char * buf, const char * base64code, int src_len) 
{   
	if (src_len == 0)
		src_len = strlen(base64code);

	int len = 0;
	unsigned char* psrc = (unsigned char*)base64code;
	char * pbuf = buf;
	int i;
	for (i = 0; i < src_len - 4; i += 4)
	{
		unsigned long ulTmp = *(unsigned long*)psrc;
		
		register int b0 = (GetB64Index((char)B0_0(ulTmp)) << 2 | GetB64Index((char)B1(ulTmp)) << 2 >> 6) & 0xFF;
		register int b1 = (GetB64Index((char)B1(ulTmp)) << 4 | GetB64Index((char)B2(ulTmp)) << 2 >> 4) & 0xFF;
		register int b2 = (GetB64Index((char)B2(ulTmp)) << 6 | GetB64Index((char)B3(ulTmp)) << 2 >> 2) & 0xFF;
		
		*((unsigned long*)pbuf) = b0 | b1 << 8 | b2 << 16;
		psrc  += 4; 
		pbuf += 3;
		len += 3;
	}

	// 处理最后余下的不足4字节的饿数据
	if (i < src_len)
	{
		int rest = src_len - i;
		unsigned long ulTmp = 0;
		for (int j = 0; j < rest; ++j)
		{
			*(((unsigned char*)&ulTmp) + j) = *psrc++;
		}
		
		register int b0 = (GetB64Index((char)B0_0(ulTmp)) << 2 | GetB64Index((char)B1(ulTmp)) << 2 >> 6) & 0xFF;
		*pbuf++ = b0;
		len  ++;

		if ('=' != B1(ulTmp) && '=' != B2(ulTmp))
		{
			register int b1 = (GetB64Index((char)B1(ulTmp)) << 4 | GetB64Index((char)B2(ulTmp)) << 2 >> 4) & 0xFF;
			*pbuf++ = b1;
			len  ++;
		}
		
		if ('=' != B2(ulTmp) && '=' != B3(ulTmp))
		{
			register int b2 = (GetB64Index((char)B2(ulTmp)) << 6 | GetB64Index((char)B3(ulTmp)) << 2 >> 2) & 0xFF;
			*pbuf++ = b2;
			len  ++;
		}

	}
		
	*pbuf = '\0'; 
	
	return len;
}


#define RSA_MODULE "00bf63d4b708b67e3926e97272ffafdf715f5fdf5f46bfa7d5e4617685c73a31c2d20cf76be6fef1d8466fedc60d93ac757adedc77800d946444a56c4b20f2a010d446ec812701a234337377a73da04a60f56986944aa380d2784b07c52fe67be45c3d1fc4a3adeeb0a994ec1e7944e9c1f5d92447f09736417362d0ae2ca4128b"
#define RSA_PUBLIC 65537
#define RSA_PRIVATE "6ea7867dc8b0d26bb2beb8281b02913a983cf305bffee147be4247677357871baf9c4595023a1693c7adc1188813005bf00d5804536a0688ae53c237f1b5b5aae42ad34385d1ca6a7d7da1bb7deaab2b4eb76eff554dc015d1de0c9fc7c0f4405d91b472e7d06c1cb5e7fbb221b422ab747fbabad804ff33d4339a7cb7621811"
/*
#define RSA_MODULE "00f5aa46574109c904fe8f2084fc7fd58b"
#define RSA_PUBLIC 65537
#define RSA_PRIVATE "76bbcc71818ab42f964ee98d4b2b4bd9"
*/
int CCipher::RsaPublicEncrypt(char *inbuf, int inlen, char *outbuf, int &outlen)
{
	BIGNUM *bnn = BN_new();
	BIGNUM *bne = BN_new();
	
	BN_hex2bn(&bnn, RSA_MODULE);
    BN_set_word(bne, RSA_PUBLIC);
	
	RSA *rsa_public = RSA_new();
    rsa_public->n = bnn;
    rsa_public->e = bne;
	
	outlen = RSA_public_encrypt(inlen, (unsigned char *)inbuf, (unsigned char *)outbuf, rsa_public, RSA_PKCS1_PADDING );

    RSA_free( rsa_public );
	
	return 0;
}

int CCipher::RsaPrivateDecrypt(char *inbuf, int inlen, char *outbuf, int &outlen)
{
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

	outlen = RSA_private_decrypt( inlen, (unsigned char *)inbuf, (unsigned char *)outbuf, rsa_private, RSA_PKCS1_PADDING );

	RSA_free( rsa_private );

	return 0;
}

}
}
