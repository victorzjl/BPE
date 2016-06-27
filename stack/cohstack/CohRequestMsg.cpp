#include "CohRequestMsg.h"
#include "UrlCode.h"
#include <boost/algorithm/string.hpp> 
#include "CohLogHelper.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

namespace sdo{
namespace coh{
void CCohRequestMsg::SetUrl(const string &strUrl)
{
    char szHost[100]={0};
    char szPath[200]={'/','\0'};
    m_dwPort=DEFAULT_COH_PORT;

    if (sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]:%u%199s", szHost, &m_dwPort, szPath)!=3)
    {
		if (sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]:%u", szHost, &m_dwPort)!=2)
        {      
			sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]%s", szHost, szPath);
        }
    }

    m_strIp= szHost;
    m_strPath=szPath;

}
void CCohRequestMsg::SetAttribute(const string &strItem, const string &strValue)
{
    m_strBody.append(strItem+"="+UrlEncoder::encode(strValue.c_str())+"&");
}
void CCohRequestMsg::SetAttribute(const string &strItem, int nValue)
{
    char szBuff[32]={0};
    sprintf(szBuff,"%d",nValue);
    SetAttribute(strItem,szBuff);
}

const string & CCohRequestMsg::Encode()
{
    char szBuf[128]={0};
	m_strRequest.append("POST "+m_strPath+" HTTP/1.0\r\n");
	m_strRequest.append("Accept: */*\r\n");
	m_strRequest.append("User-Agent: COH Client/1.0\r\n");
    snprintf(szBuf,127,"Host: %s:%d\r\n",m_strIp.c_str(),m_dwPort);
    m_strRequest.append(szBuf);
    m_strRequest.append("Connection: close\r\n");

	if(m_strBody.length()>0)
	{
	    m_strBody.erase(m_strBody.length()-1);
		sprintf(szBuf,"Content-length: %d\r\n\r\n",m_strBody.length());
		m_strRequest.append("Content-type: application/x-www-form-urlencoded\r\n");
		
		m_strRequest.append(szBuf);
		m_strRequest.append(m_strBody);
	}
	else
	{
		m_strRequest.append("\r\n");
	}
	return m_strRequest;
}

int CCohRequestMsg::Decode(const string &strRequest)
{
    size_t begin,end;
	int nBodyLen=0;
	if((end = strRequest.find("\r\n"))==string::npos||(end = strRequest.find("\n"))==string::npos)
    {
        return -1;
    }
	char url_buf[2048]={0};
    if(sscanf(strRequest.substr(0,end).c_str(),"GET %2047s",url_buf)==1)
    {
        char szCommand[128]={0};
        char szAttribute[2048]={0};
        int n=sscanf(url_buf,"%[^?]?%s",szCommand,szAttribute);
        if(n==1)
        {
            m_strPath=szCommand;
        }
        else if(n==2)
        {
            m_strPath=szCommand;
            string strBosy=szAttribute;
            vector<string> vecFields;
            boost::algorithm::split( vecFields, 
            strBosy,boost::algorithm::is_any_of("&"),boost::algorithm::token_compress_on); 
            vector<string>::iterator itr;
            for(itr=vecFields.begin();itr!=vecFields.end();itr++)
            {
                string strField=*itr;
                size_t loc=strField.find('=');
                string strKey=strField.substr(0,loc);
                string strValue=strField.substr(loc+1);;
                m_mapAttri.insert(boost::unordered_multimap<string,string>::value_type(strKey,URLDecoder::decode(strValue.c_str())));
            }
        }
        return 0;
    }
	else if(sscanf(strRequest.substr(0,end).c_str(),"POST %2047s",url_buf)==1)
	{
		m_strPath=url_buf;
		char szHead[64]={0};
		char szHeadValue[128]={0};
		while (1) 
        {   
			begin=end;
			end = strRequest.find("\r\n",begin+2);
			if (end==string::npos)
				break;
			if(sscanf(strRequest.substr(begin+2,end-begin-2).c_str(),"%63[^:]:%127s",
szHead,szHeadValue)<2)
			{
				break;
			}
			if(strncmp(szHead,"Content-Length",14)==0||strncmp(szHead,"Content-length",14)
==0)
			{
				nBodyLen=atoi(szHeadValue);
			}
		}	
		if (nBodyLen>0) 
		{
		    string strBosy=strRequest.substr(begin+4,nBodyLen);
            vector<string> vecFields;
            boost::algorithm::split( vecFields, 
            strBosy,boost::algorithm::is_any_of("&"),boost::algorithm::token_compress_on); 
            vector<string>::iterator itr;
            for(itr=vecFields.begin();itr!=vecFields.end();itr++)
            {
                string strField=*itr;
                size_t loc=strField.find('=');
                string strKey=strField.substr(0,loc);
                string strValue=strField.substr(loc+1);;
                m_mapAttri.insert(boost::unordered_multimap<string,string>::value_type(strKey,URLDecoder::decode(strValue.c_str())));
            }
            
		}
        return 0;
	}
	else
	{
		return -1;
	}}
string CCohRequestMsg::GetCommand() const
{
    size_t begin;
    if((begin=m_strPath.rfind('/'))!=string::npos)
        return m_strPath.substr(begin+1);
    else
        return m_strPath;
}
string CCohRequestMsg::GetAttribute(const string &strItem) const
{
    boost::unordered_multimap<string,string>::const_iterator itr=m_mapAttri.find(strItem);
    if(itr!=m_mapAttri.end())
        return itr->second;
    else
        return "";
}
vector<string> CCohRequestMsg::GetAttributes(const string &strItem) const
{
    std::vector<string> vec;
    std::pair<boost::unordered_multimap<string,string>::const_iterator, boost::unordered_multimap<string,string>::const_iterator> itr_pair = 
        m_mapAttri.equal_range(strItem);
    boost::unordered_multimap<string,string>::const_iterator itr;
	for(itr=itr_pair.first; itr!=itr_pair.second;itr++)
	{
		vec.push_back(itr->second);
	}
	return vec;
}

}
}

