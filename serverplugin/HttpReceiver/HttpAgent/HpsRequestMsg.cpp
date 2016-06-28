#include "HpsRequestMsg.h"
#include <boost/algorithm/string.hpp> 
#include "HpsLogHelper.h"
#include "UrlCode.h"
#include "CommonMacro.h"

namespace sdo{
namespace hps{
CHpsRequestMsg::CHpsRequestMsg():m_isKeepAlive(false),m_isXForwardedFor(false),m_nHttpVersion(0),m_xffport(0)
{
}

char *CHpsRequestMsg::FindLastString(char* pBegin,char* pEnd,const char* pSub)
{
	char* pLastSpace = NULL;
	char *pSpace = strstr(pBegin,pSub);
	while (pSpace!=NULL && pSpace<=pEnd)
	{
		pLastSpace = pSpace;
		pSpace = strstr(pSpace+1,pSub);
	}
	return pLastSpace;
}

int CHpsRequestMsg::Decode(char *pBuff, int nLen)
{
	int nBodyLen=0;
	char szHead[64]={0};
	char szHeadValue[128]={0};

    char *pBegin=pBuff;
    char *pEnd=strstr(pBegin,"\r\n");
    if(pEnd==NULL||pEnd-pBuff>=nLen)
		return -1;

	char szHttpVersion[256] = {0};
	
    if(0==strncmp(pBuff, "GET", 3))
    { 
        m_isPost = false;
		pBegin += 4;
		char *pSpace = FindLastString(pBegin,pEnd," ");
		if(pSpace==NULL || pSpace>=pEnd)
		{
			HP_XLOG(XLOG_ERROR,"CHpsRequestMsg::%s, DecodeFail %s\n",__FUNCTION__,pBegin);	
			return -1;
		}
		char *pSplit = strstr(pBegin,"?");
		if(pSplit!=NULL && pSplit<pSpace)
		{
			m_strPath = string(pBegin,pSplit-pBegin);
			m_strBody = string(pSplit+1, pSpace-pSplit-1);
		}
		else
		{
			m_strPath = string(pBegin,pSpace-pBegin);
		}
		
		pBegin = pSpace + 1;
		memcpy(szHttpVersion,pBegin,pEnd-pBegin);
		
        if(strncasecmp(szHttpVersion, "http/1.1", 8) == 0)
		{
			m_nHttpVersion = 1;
			m_isKeepAlive = true;
		}
        else
		{
			m_nHttpVersion = 0;
			m_isKeepAlive = false;
		}

        pBegin = strstr(pBegin, "\r\n") + 2;
        const char *pHeadersEnd = strstr(pBegin, "\r\n\r\n");
        ParseHeaders(pBegin, pHeadersEnd - pBegin + 4);
    }
    else if(0==strncmp(pBuff, "POST", 4))
    {
        m_isPost = true;
		pBegin += 5;
		char *pSpace = FindLastString(pBegin,pEnd," ");
		if(pSpace==NULL || pSpace>=pEnd)
		{
			HP_XLOG(XLOG_ERROR,"CHpsRequestMsg::%s, DecodeFail %s\n",__FUNCTION__,pBegin);	
			return -1;
		}
		char *pSplit = strstr(pBegin,"?");
		if(pSplit!=NULL && pSplit<pSpace)
		{
			m_strPath = string(pBegin,pSplit-pBegin);
			m_strBody = string(pSplit+1, pSpace-pSplit-1);
		}
		else
		{
			m_strPath = string(pBegin,pSpace-pBegin);
		}

		pBegin = pSpace + 1;
		memcpy(szHttpVersion,pBegin,pEnd-pBegin);

        if(strncasecmp(szHttpVersion, "http/1.1", 8) == 0)
		{
			m_nHttpVersion = 1;
			m_isKeepAlive = true;
		}
        else
		{
			m_nHttpVersion = 0;
			m_isKeepAlive = false;
		}
        pBegin = strstr(pBegin, "\r\n") + 2;
        const char *pHeadersEnd = strstr(pBegin, "\r\n\r\n");
        ParseHeaders(pBegin, pHeadersEnd - pBegin + 4);

        
        if (m_dwContentLen>0)
		{
			if (m_strBody.length() > 0)
				m_strBody = string("&") + m_strBody;
            m_strBody = string(pBuff + nLen - m_dwContentLen, m_dwContentLen) + m_strBody;
			HP_XLOG(XLOG_TRACE,"CHpsRequestMsg::%s,m_strBody[%s] len[%d]\n",__FUNCTION__,m_strBody.c_str(),m_strBody.length());			
		}
    }
	else
	{
		return -1;
	}
    return 0;
}

void CHpsRequestMsg::ParseHeaders(const char *pHeaders, unsigned int dwLen)
{
    m_dwContentLen = 0;
    char szHead[64] = { 0 };
    char szHeadValue[128] = { 0 };
    const char *pBegin = pHeaders;
    const char *pEnd = strstr(pBegin, "\r\n");
    while (pBegin != pEnd)
    {
        if (sscanf(pBegin, "%63[^:]: %127[^\r]", szHead, szHeadValue) == 2)
        {
            if (strncasecmp(pBegin, "cookie", 6) == 0)
            {
                m_strCookie = szHeadValue;
                pBegin = pEnd + 2;
                pEnd = strstr(pBegin, "\r\n");
                continue;
            }
            else if ((strncasecmp(szHead, "connection", 10) == 0))
            {
                if (strncasecmp(szHeadValue, "close", 6) == 0)
                {
                    m_isKeepAlive = false;
                }
                else if (strncasecmp(szHeadValue, "keep-alive", 10) == 0)
                {
                    m_isKeepAlive = true;
                }
            }
            else if (strncasecmp(szHead, "x-forwarded-for", 16) == 0)
            {
                char szIp[128] = { 0 };
                char *pLocate = strchr(szHeadValue, ',');
                if (NULL != pLocate)
                {
                    memcpy(szIp, szHeadValue, (int)(pLocate - szHeadValue));
                }
                else
                {
                    memcpy(szIp, szHeadValue, strlen(szHeadValue));
                }
                m_strXForwardedFor = szIp;
                if (m_strXForwardedFor.size() > 0)
                    m_isXForwardedFor = true;
            }
            else if (strncasecmp(szHead, "host", 4) == 0)
            {
                m_strHost = szHeadValue;
            }
            else if (strncasecmp(szHead, "x-ff-port", 9) == 0)
            {
                m_xffport = atoi(szHeadValue);
            }
            else if (strncasecmp(szHead, "SOAPAction", 10) == 0) // support webservice
            {
                m_strPath = szHeadValue;
                if (m_strPath.find("\"") == 0)
                    m_strPath = m_strPath.substr(1);
                if (m_strPath.find("\"") + 1 == m_strPath.length())
                    m_strPath = m_strPath.substr(0, m_strPath.length() - 1);
                m_strBody = "";
            }
            else if (strncasecmp(szHead, "user-agent", 10) == 0)
            {
                m_strUserAgent = szHeadValue;
            }
            else if (strncasecmp(szHead, "content-type", 12) == 0)
            {
                m_strContentType = szHeadValue;
            }
            else if (strncasecmp(szHead, "content-length", 14) == 0)
            {
                m_dwContentLen = atoi(szHeadValue);
            }

            m_headers.insert(std::make_pair(szHead, szHeadValue));
            pBegin = pEnd + 2;
            pEnd = strstr(pBegin, "\r\n");
        }
    }
}

string CHpsRequestMsg::GetCommand() const
{
	if(m_strPath.length() > 0)
	{
        if (m_strPath.at(0) == '/')
        {
            return m_strPath.substr(1);
        }
	}

    return m_strPath;
}

string CHpsRequestMsg::GetValueByKey(const string& strUrlArri, const string& strKey)
{
	string strResult;
	string strRealKey = "&" + strKey + "=";
	string::size_type posKey = strUrlArri.find(strRealKey);
	if (posKey == string::npos)
	{
		strRealKey = strKey + "=";
		if (strncmp(strUrlArri.c_str(), strRealKey.c_str(), strRealKey.length()) == 0)
		{
			posKey = 0;
		}
	}
	if (posKey != string::npos)
	{
		string::size_type posVelueFrom = posKey+strRealKey.length();
		string::size_type posValueEnd = strUrlArri.find('&', posVelueFrom);
		strResult = 
			(posValueEnd != string::npos) ? 
			strUrlArri.substr(posVelueFrom, posValueEnd-posVelueFrom) :
		strUrlArri.substr(posVelueFrom, strUrlArri.length()-posVelueFrom);
	}
	return URLDecoder::decode(strResult.c_str());
}


}
}

