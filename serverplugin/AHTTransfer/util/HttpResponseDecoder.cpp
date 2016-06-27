#include "HttpResponseDecoder.h"
#include "SoPluginLog.h"
#ifdef WIN32
#define snprintf _snprintf
#ifndef strncasecmp
#define strncasecmp strnicmp 
#endif
#endif

static void toLower(char *pStr)
{
	while ((*pStr) != '\0')
	{
        if ((*pStr)>='A' && (*pStr)<='Z')
		{
            *pStr += 32;
        }
        pStr++;
    }
}

static char* trim(char *pSrcStr)
{
	char *pBegin = pSrcStr;
	char *pEnd = pSrcStr + strlen(pSrcStr) - 1;
	
	while ((*pBegin) != '\0' && pBegin < pEnd)
	{
        if ((*pBegin) != ' ')
		{
			break;
        }		
        pBegin++;
    }
	
	while(pEnd >= pBegin)
	{
		if ((*pEnd) != ' ')
		{
			break;
        }	
		*pEnd = '\0';
		pEnd--;
	}

	return pBegin > pSrcStr ? pBegin : pSrcStr;
}

int CHttpResponseDecoder::Decode(const char *pBuffer, int nLen)
{
	SO_XLOG(XLOG_DEBUG,"CHttpResponseDecoder::%s::%d  nLen[%d]  [\n%s\n]\n",__FUNCTION__,__LINE__,nLen,pBuffer);
	char *pHeadEnd = strstr(pBuffer, "\r\n\r\n");
	if(pHeadEnd == NULL || pHeadEnd-pBuffer > nLen)
	{
		return -1;
	}
	
	const char *pBegin = pBuffer;
	char szVersion[8] = {0};
	char szMsg[32] = {0};
	if(3 != sscanf(pBegin, "HTTP/%7s %d %31[^\r\n]",szVersion, &m_nHttpCode, szMsg))
	{
		return -2;
	}	
	
	int nBodyLen=0;
	bool isChunked = false;
    
    pBegin = strstr(pBegin,"\r\n");
	while (pBegin != NULL && pBegin-pBuffer <= nLen && pBegin < pHeadEnd) 
    {   
		char szHead[128]={0};
		char szValue[512]={0};
		
		pBegin += 2;
		if(sscanf(pBegin,"%127[^:]:%511[^\r\n]",szHead,szValue)!=2)
		{
			pBegin = strstr(pBegin,"\r\n");
			continue;
		}
		char *pHead = trim(szHead);
		char *pValue = trim(szValue);
		toLower(pHead);
		m_mapHeadValue.insert(make_pair(pHead,pValue));		
		if(0==strncmp(pHead,"content-length",14))
		{
			nBodyLen=atoi(pValue);			
		}
		else if((0==strncmp(pHead,"transfer-encoding",17))&&(0==strncmp(pValue,"chunked",7)))
		{
			isChunked = true;			
		}		
        pBegin = strstr(pBegin, "\r\n");	
	}	

	if(isChunked)
	{
		pBegin = pHeadEnd + 2;
		char *pBodyEnd = strstr(pBegin,"\r\n0\r\n");
		while(pBegin != NULL && pBodyEnd != NULL && (pBegin <= pBodyEnd) && (pBegin-pBuffer <= nLen))		
		{ 
			pBegin += 2;
			char szLen[32] = {0};
			int nLength = 0;		
			int nRet = sscanf(pBegin, "%31[^\r\n]", szLen);
			if(nRet < 1 || NULL == (pBegin = strstr(pBegin,"\r\n")))
			{
				return -2;
			}	
				
			sscanf(szLen, "%0X", &nLength);
					
			pBegin += 2;	
			if((pBegin + nLength > pBodyEnd+5) || (pBegin + nLength-pBuffer > nLen))
			{
				SO_XLOG(XLOG_WARNING,"CHttpResponseDecoder::%s http response illegal.   pBuffer[0X%0X], nLen[%d], pBegin[0X%0X],nLength[%d],pBodyEnd[0X%0X]\n",
						__FUNCTION__,pBuffer,nLen,pBegin,nLength,pBodyEnd+5);
				return -3;
			}
			
			m_strBody += string(pBegin , nLength);
			
			pBegin = pBegin + nLength;			
			pBegin = strstr(pBegin,"\r\n");			
		}
	}
	else
	{
		if (nBodyLen>0) 
		{
		    m_strBody = string(pHeadEnd+4,nBodyLen);            
		}
		else
		{
			int nRest = pBuffer + nLen - pHeadEnd -4;
			if(nRest > 0)
			{
				m_strBody = string(pHeadEnd+4,nRest);
			}
			else
			{
				SO_XLOG(XLOG_WARNING,"CHttpResponseDecoder::%s http response illegal.   pBuffer[0X%0X], nLen[%d], pHeadEnd[0X%0X]\n",__FUNCTION__,pBuffer,nLen,pHeadEnd);
				SO_XLOG(XLOG_WARNING,"CHttpResponseDecoder::%s::%d  nLen[%d]  [\n%s\n]\n",__FUNCTION__,__LINE__,nLen,pBuffer);
				return -4;
			}
		}
	}
	return 0;
	
}

string CHttpResponseDecoder::GetHeadValue(const string &strHead)
{
	map<string,string>::iterator itr = m_mapHeadValue.find(strHead);
	if(itr != m_mapHeadValue.end())
	{
		return itr->second;
	}
	else
	{
		return "";
	}
}

void CHttpResponseDecoder::Dump()
{
	SO_XLOG(XLOG_DEBUG,"=========CHttpResponseDecoder::DUMP()=======\n");
	map<string,string>::iterator itr;
	for(itr = m_mapHeadValue.begin(); itr != m_mapHeadValue.end(); ++itr)
	{
		SO_XLOG(XLOG_DEBUG,"%s:%s\n", itr->first.c_str(),itr->second.c_str());
	}

	SO_XLOG(XLOG_DEBUG,"http code[%d]\n", m_nHttpCode);
	SO_XLOG(XLOG_DEBUG,"http body:[%s]\n", m_strBody.c_str());
	SO_XLOG(XLOG_DEBUG,"=========~CHttpResponseDecoder::DUMP()=======\n");
}
