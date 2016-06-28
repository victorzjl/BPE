#include "HttpResponseDecoder.h"

#ifdef WIN32
#define snprintf _snprintf
#ifndef strncasecmp
#define strncasecmp strnicmp 
#endif
#endif


int CHttpResponseDecoder::Decode(char *pBuffer, int nLen)
{
	char *pHeadEnd = strstr(pBuffer, "\r\n\r\n");
	if(pHeadEnd == NULL || pHeadEnd-pBuffer > nLen)
	{
		return -1;
	}
	
	char *pBegin = pBuffer;
	char szVersion[8] = {0};
	char szMsg[32] = {0};
	if(3 != sscanf(pBegin, "HTTP/%7s %d %31[^\r\n]",szVersion, &m_nHttpCode, szMsg))
	{
		return -1;
	}	
	pBegin = strstr(pBegin,"\r\n");

	int nBodyLen=0;
	bool isChunked = false;
	

		
	while (pBegin != NULL && pBegin-pBuffer <= nLen && pBegin < pHeadEnd) 
    {   
		char szHead[128]={0};
		char szValue[512]={0};
		
		pBegin += 2;
		if(sscanf(pBegin,"%127[^:]:%511[^\r\n]",szHead,szValue)!=2)
		{
			pBegin = strstr(pBegin+2,"\r\n");
			continue;
		}
		m_mapHeadValue.insert(make_pair(szHead,szValue));
		
		pBegin = strstr(pBegin, "\r\n");	
		
		if(0==strncasecmp(szHead,"Content-Length",14))
		{
			nBodyLen=atoi(szValue);
			continue;
		}
		if((0==strncasecmp(szHead,"Transfer-Encoding",17))&&(0==strncasecmp(szValue,"chunked",7)))
		{
			isChunked = true;
			continue;
		}		
	}	

	if(isChunked)
	{
		pBegin = pHeadEnd + 2;
		char *pBodyEnd = strstr(pBegin,"\r\n0\r\n\r\n");
		while(pBegin != NULL && pBodyEnd != NULL && pBegin < pBodyEnd)		
		{
			pBegin += 2;
			int nLength;		
			int nRet = sscanf(pBegin, "%0X[^\r\n]", &nLength);
			if(NULL == (pBegin = strstr(pBegin,"\r\n")) || nRet < 1 )
				return -2;
			
			pBegin += 2;			
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
				return -3;
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
	map<string,string>::iterator itr;
	for(itr = m_mapHeadValue.begin(); itr != m_mapHeadValue.end(); ++itr)
	{
		fprintf(stdout,"%s:%s\n", itr->first.c_str(),itr->second.c_str());
	}
}