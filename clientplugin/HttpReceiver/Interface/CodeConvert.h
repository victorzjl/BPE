#ifndef _CODE_CONVERT_H_
#define _CODE_CONVERT_H_
#include "IAvenueHttpTransfer.h"

#include <iconv.h>
#include <stdlib.h>
#include <string>
using std::string;

#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE 102400
#endif

class CCodeConverter
{
public:
	CCodeConverter()
	{
		m_pInBuffer = (char *)malloc(MAX_BUFFER_SIZE);
		m_pOutBuffer = (char *)malloc(MAX_BUFFER_SIZE);
		memset(m_pInBuffer, 0, MAX_BUFFER_SIZE);
		memset(m_pOutBuffer, 0, MAX_BUFFER_SIZE);
		m_iconvUtf82Gbk = iconv_open("gbk","utf-8");
		if(m_iconvUtf82Gbk == NULL)
		{
			SO_XLOG(XLOG_ERROR,"create icon_t fail. iconv_open('gbk','utf-8') failed\n");
		}
		m_iconvGbk2Utf8 = iconv_open("utf-8", "gbk");
		if(m_iconvGbk2Utf8 == NULL)
		{
			SO_XLOG(XLOG_ERROR,"create icon_t fail. iconv_open('utf-8', 'gbk') failed\n");
		}
	}
	
	string Utf82Gbk(const string &strIn)
	{
		return convert(m_iconvUtf82Gbk, strIn);
	}
	
	string Gbk2Utf8(const string &strIn)
	{
		return convert(m_iconvGbk2Utf8, strIn);
	}
	
	~CCodeConverter()
	{
		if(m_iconvUtf82Gbk != NULL)
		{
			iconv_close(m_iconvUtf82Gbk);
		}
		if(m_iconvGbk2Utf8 != NULL)
		{
			iconv_close(m_iconvGbk2Utf8);
		}
		free(m_pInBuffer);
		free(m_pOutBuffer);
	}
	
private:
	string convert(iconv_t ic, const string &strIn)
	{
		memset(m_pOutBuffer, 0, MAX_BUFFER_SIZE);
		memset(m_pInBuffer, 0, MAX_BUFFER_SIZE);
		memcpy(m_pInBuffer, strIn.c_str(), strIn.size());
		
		char *pInBuf = m_pInBuffer;
		char *pOutBuf = m_pOutBuffer;
		char **ppInBuf = (char **)&pInBuf;
		char **ppOutBuf = (char **)&pOutBuf;
		
		size_t nInLength = strIn.size();
		size_t nOutBufferSize = MAX_BUFFER_SIZE - 128;
		
		if(ic != NULL)
		{
#ifdef WIN32
			size_t ret = iconv (ic, (const char **)ppInBuf, &nInLength, ppOutBuf, &nOutBufferSize);
#else
            size_t ret = iconv (ic, ppInBuf, &nInLength, ppOutBuf, &nOutBufferSize);
#endif
			if(ret >= 0)
			{
				return string(m_pOutBuffer);
			}
			else
			{
				SO_XLOG(XLOG_WARNING,"iconv fail. %s\n", strIn.c_str());
				return strIn;
			}
		}
		else
		{
			SO_XLOG(XLOG_WARNING,"iconv fail. ic is NULL %s\n", strIn.c_str());
			return strIn;
		}
	}

private:
	iconv_t m_iconvUtf82Gbk;
	iconv_t m_iconvGbk2Utf8;
	
	char *m_pInBuffer;
	char *m_pOutBuffer;
};

#endif
