#ifndef _CODE_CONVERTER_H_
#define _CODE_CONVERTER_H_

#include <iconv.h>
#include <iostream>
#include <boost/asio.hpp>
#define OUTLEN 255

using namespace std;


class CodeConverter
{
public:
	CodeConverter(const char *FromCharset,const char *ToCharset)
	{
		m_ConvertCode = iconv_open(ToCharset,FromCharset);
	}

	~CodeConverter()
	{
		iconv_close(m_ConvertCode);
	}
	int Convert(char *szSrcBuf,int iSrcLen,char *szDestbuf,int* iDestlen)
	{
		char **pin = &szSrcBuf;
		char **pout = &szDestbuf;
		memset(szDestbuf,0,*iDestlen);
		int len = *iDestlen;
		int nRet = 0;
#ifdef WIN32
		nRet = iconv(m_ConvertCode,(const char **)pin,(size_t *)&iSrcLen,pout,(size_t *)iDestlen);
#else
		nRet = iconv(m_ConvertCode,pin,(size_t *)&iSrcLen,pout,(size_t *)iDestlen);
#endif
		*iDestlen = len  - *iDestlen;
		return nRet;
	}
private:
	iconv_t m_ConvertCode;
};

class Ascii2Utf8
{
public:
	Ascii2Utf8()
		: m_ConvertCode("gbk", "utf-8")
		, m_ConvertCode2("gb18030", "utf-8")
	{
	}
	int Convert(char *src_buff,int str_len,char *dest_buff,int dest_len)
	{
		char* start = src_buff;
		char* end;
		char *pout = dest_buff;
		while (start < (src_buff + str_len))
		{
			end = start;
			while (*(unsigned char*)end != 0x80 && 
				*(unsigned char*)end != 0xff && 
				*(unsigned char*)end < 0xfe && 
				ntohs(*(unsigned short*)end) != 0xa8bf &&
				!(ntohs(*(unsigned short*)end) >= 0xa989 && ntohs(*(unsigned short*)end) <= 0xa995) &&
				(unsigned char*)end < ((unsigned char*)src_buff + str_len))
			{
				if (*(unsigned char*)end >= 0x81)
					end += 2;
				else
					end += 1;
			}
			
			if (end - start > 0)
			{
				int slen = end - start;
				int dlen = dest_len - (pout - dest_buff);
				int ret = m_ConvertCode.Convert(start, slen, pout, &dlen);
				if (ret == -1)
				{
					dlen = dest_len - (pout - dest_buff);
					ret = m_ConvertCode2.Convert(start, slen, pout, &dlen);
				}
				if (dlen > 0)
					pout += dlen;
				start = end;
			}
			
			if (dest_len - (pout - dest_buff) < 3)
				break;
			
			if ((unsigned char*)end < ((unsigned char*)src_buff + str_len))
			{
				if (*(unsigned char*)end == 0x80)
				{
					pout[0] = 0xe2;
					pout[1] = 0x82;
					pout[2] = 0xac;
					pout += 3;
					start += 1;
				}
				else if (*(unsigned char*)end == 0xff)
				{
					pout[0] = 0xef;
					pout[1] = 0xa3;
					pout[2] = 0xb5;
					pout += 3;
					start += 1;
				}
				else if (ntohs(*(unsigned short*)end) == 0xa8bf)
				{
					pout[0] = 0xee;
					pout[1] = 0x9f;
					pout[2] = 0x88;
					pout += 3;
					start += 2;
				}
				else if (ntohs(*(unsigned short*)end) >= 0xa989 && ntohs(*(unsigned short*)end) <= 0xa995)
				{
					unsigned char src = *(end + 1);
					pout[0] = 0xee;
					pout[1] = 0x9f;
					pout[2] = 0xa7 + src - 0x89;
					pout += 3;
					start += 2;
				}
				else
				{
					if (((unsigned char*)end + 1) >= ((unsigned char*)src_buff + str_len))
						break;

					unsigned char src = *(end + 1);
					unsigned short dest;
					if (src <= 0x7a)
						dest = ntohs(0xa095 - 0x50 + src);
					else if (src > 0x7a && src <= 0x7e)
						dest = ntohs(0xa180 - 0x7b + src);
					else
						dest = ntohs(0xa184 - 0x80 + src);
					*pout = 0xee;
					*(unsigned short*)(pout+1) = dest;
					start += 2;
					pout += 3;
				}
			}
		}
		return pout - dest_buff;
	}

private:
	CodeConverter m_ConvertCode;
	CodeConverter m_ConvertCode2;
};

class Utf82Ascii
{
public:
	Utf82Ascii()
		: m_ConvertCode("utf-8", "gbk")
		, m_ConvertCode2("utf-8", "gb18030")
	{
	}
	int Convert(char *src_buff,int str_len,char *dest_buff,int dest_len)
	{
		char* start = src_buff;
		char* end;
		char *pout = dest_buff;
		while (start < (src_buff + str_len))
		{
			end = start;
			while ((unsigned char*)end < ((unsigned char*)src_buff + str_len) &&
				!(*(unsigned char*)end == 0xee && ntohs(*(unsigned short*)(end+1)) >= 0xa095 && ntohs(*(unsigned short*)(end+1)) <= 0xa1a4) &&
				!(*(unsigned char*)end == 0xee && ntohs(*(unsigned short*)(end+1)) >= 0x9fa7 && ntohs(*(unsigned short*)(end+1)) <= 0x9fb3) &&
				!(*(unsigned char*)end == 0xe2 && ntohs(*(unsigned short*)(end+1)) == 0x82ac) &&
				!(*(unsigned char*)end == 0xef && ntohs(*(unsigned short*)(end+1)) == 0xa3b5) &&
				!(*(unsigned char*)end == 0xee && ntohs(*(unsigned short*)(end+1)) == 0x9f88))
			{
				if (*(unsigned char*)end < 0x80)
					end += 1;
				else if (*(unsigned char*)end < (0x80 + 0x40 + 0x20))
					end += 2;
				else
					end += 3;
			}
			
			if (end - start > 0)
			{
				int slen = end - start;
				int dlen = dest_len - (pout - dest_buff);
				int ret = m_ConvertCode.Convert(start, slen, pout, &dlen);
				if (ret == -1)
				{
					dlen = dest_len - (pout - dest_buff);
					ret = m_ConvertCode2.Convert(start, slen, pout, &dlen);
				}
				if (dlen > 0)
					pout += dlen;
				start = end;
			}
			
			if (((unsigned char*)end + 2) >= ((unsigned char*)src_buff + str_len))
				break;
				
			if ((unsigned char*)end < ((unsigned char*)src_buff + str_len))
			{
				if (*(unsigned char*)end == 0xee && ntohs(*(unsigned short*)(end+1)) >= 0xa095 && ntohs(*(unsigned short*)(end+1)) <= 0xa1a4)
				{
					if (dest_len - (pout - dest_buff) >= 3)
					{
						pout[0] = 0xfe;
						if (ntohs(*(unsigned short*)(end+1)) >= 0xa095 && ntohs(*(unsigned short*)(end+1)) <= 0xa0bf)
							pout[1] = (unsigned char)(ntohs(*(unsigned short*)(end+1)) - 0xa095 + 0x50);
						else if (ntohs(*(unsigned short*)(end+1)) >= 0xa180 && ntohs(*(unsigned short*)(end+1)) <= 0xa183)
							pout[1] = (unsigned char)(ntohs(*(unsigned short*)(end+1)) - 0xa180 + 0x7b);
						else
							pout[1] = (unsigned char)(ntohs(*(unsigned short*)(end+1)) - 0xa184 + 0x80);
						pout += 2;
						start += 3;
					}
					else
					{
						break;
					}
				}
				else  if (*(unsigned char*)end == 0xef && ntohs(*(unsigned short*)(end+1)) == 0xa3b5)
				{
					if (dest_len - (pout - dest_buff) >= 1)
					{
						pout[0] = 0xff;
						pout += 1;
						start += 3;
					}
					else
					{
						break;
					}
				}
				else if (*(unsigned char*)end == 0xe2 && ntohs(*(unsigned short*)(end+1)) == 0x82ac)
				{
					if (dest_len - (pout - dest_buff) >= 1)
					{
						pout[0] = 0x80;
						pout += 1;
						start += 3;
					}
					else
					{
						break;
					}
				}
				else if (*(unsigned char*)end == 0xee && ntohs(*(unsigned short*)(end+1)) == 0x9f88)
				{
					if (dest_len - (pout - dest_buff) >= 1)
					{
						pout[0] = 0xa8;pout[1] = 0xbf;
						pout += 2;
						start += 3;
					}
					else
					{
						break;
					}
				}
				else if (*(unsigned char*)end == 0xee && ntohs(*(unsigned short*)(end+1)) >= 0x9fa7 && ntohs(*(unsigned short*)(end+1)) <= 0x9fb3)
				{
					if (dest_len - (pout - dest_buff) >= 1)
					{
						pout[0] = 0xa9;
						pout[1] = (unsigned char)(ntohs(*(unsigned short*)(end+1)) - 0x9fa7 + 0x89);
						pout += 2;
						start += 3;
					}
					else
					{
						break;
					}
				}
			}
		}
		return pout - dest_buff;
	}

private:
	CodeConverter m_ConvertCode;
	CodeConverter m_ConvertCode2;
};

/*
class CodeConverter
{
public:
	CodeConverter(const char *FromCharset,const char *ToCharset)
	{
		m_ConvertCode = iconv_open(ToCharset,FromCharset);
	}

	~CodeConverter()
	{
		iconv_close(m_ConvertCode);
	}
	int Convert(char *szSrcBuf,int iSrcLen,char *szDestbuf,int iDestlen)
	{
		char **pin = &szSrcBuf;
		char **pout = &szDestbuf;
		memset(szDestbuf,0,iDestlen);
#ifdef WIN32
		return iconv(m_ConvertCode,(const char **)pin,(size_t *)&iSrcLen,pout,(size_t *)&iDestlen);
#else
		return iconv(m_ConvertCode,pin,(size_t *)&iSrcLen,pout,(size_t *)&iDestlen);
#endif
	}
private:
	iconv_t m_ConvertCode;
};

class Ascii2Utf8
{
public:
	Ascii2Utf8()
		: m_ConvertCode("gbk", "utf-8")
		, m_ConvertCode2("gb18030", "utf-8")
	{
	}
	int Convert(char *szSrcBuf,int iSrcLen,char *szDestbuf,int iDestlen)
	{
		if (-1 == m_ConvertCode.Convert(szSrcBuf, iSrcLen, szDestbuf, iDestlen))
			return m_ConvertCode2.Convert(szSrcBuf, iSrcLen, szDestbuf, iDestlen);
		return 0;
	}

private:
	CodeConverter m_ConvertCode;
	CodeConverter m_ConvertCode2;
};

class Utf82Ascii
{
public:
	Utf82Ascii()
		: m_ConvertCode("utf-8", "gbk")
		, m_ConvertCode2("utf-8", "gb18030")
	{
	}
	int Convert(char *szSrcBuf,int iSrcLen,char *szDestbuf,int iDestlen)
	{
		if (-1 == m_ConvertCode.Convert(szSrcBuf, iSrcLen, szDestbuf, iDestlen))
			return m_ConvertCode2.Convert(szSrcBuf, iSrcLen, szDestbuf, iDestlen);
		return 0;
	}

private:
	CodeConverter m_ConvertCode;
	CodeConverter m_ConvertCode2;
};
*/
#endif

