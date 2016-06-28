#include "HttpRecConfig.h"
#include "AsyncVirtualClientLog.h"
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

// 'CN' is short for 'configure name', and 'CDV' is short for 'default configure value'
#define CN_REPORT_URL                "ReportUrl"
#define CN_DETAIL_REPORT_URL         "DetailReportUrl"
#define CN_DSG_URL                   "DgsUrl"
#define CN_ERR_REPORT_URL            "ErrReportUrl"
#define CN_WHITE_VALIDATE_LIST       "WhiteValidateList/Item"
#define CN_HTTP_THREAD               "HTTP/Thread"
#define CN_HTTP_PORT                 "HTTP/Port"
#define CN_HTTP_OSAP_OPTION          "HTTP/OsapOption"
#define CN_HTTP_OSAP_SERVER_ADDRS    "HTTP/OsapServerAddrs"
#define CN_HTTP_LOACLHOST            "HTTP/LocalHost"
#define CN_HTTP_ACCESS               "HTTP/Access"
#define CN_HTTP_H2A_DIR              "HTTP/TransferLibsDir/Http2AvenueLibDir"
#define CN_HTTP_H2A_FILE_REGEX       "HTTP/TransferLibsDir/Http2AvenueFileFilter"
#define CN_HTTP_A2H_DIR              "HTTP/TransferLibsDir/Avenue2HttpLibDir"
#define CN_HTTP_A2H_FILE_REGEX       "HTTP/TransferLibsDir/Avenue2HttpFileFilter"

#define CDV_REPORT_URL               "http://api.monitor.sdo.com/stat_more_actions.php"
#define CDV_DETAIL_REPORT_URL        "http://api.monitor.sdo.com/stat_pages.php"
#define CDV_DSG_URL                  "http://27.0.0.1/"
#define CDV_ERR_REPORT_URL           "http://api.monitor.sdo.com/error_report.php"
#define CDV_HTTP_THREAD              8
#define CDV_HTTP_PORT                "80"
#define CDV_HTTP_OSAP_OPTION         0
#define CDV_HTTP_OSAP_SERVER_ADDRS   ""
#define CDV_HTTP_LOACLHOST           "localhost,127.0.0.1"
#define CDV_HTTP_AGENT_REMOTE_ADDR   ""
#define CDV_HTTP_H2A_DIR             "./plugin"
#define CDV_HTTP_H2A_FILE_REGEX      "*.so"
#define CDV_HTTP_A2H_DIR             "./plugin_ReqA2H"
#define CDV_HTTP_A2H_FILE_REGEX      "*.so"


HttpRecConfigPtr CHttpRecConfig::s_pInstance(new CHttpRecConfig);

HttpRecConfigPtr CHttpRecConfig::GetInstance()
{
	return s_pInstance;
}

CHttpRecConfig::~CHttpRecConfig()
{
}

bool CHttpRecConfig::Load(const string strFile)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s File[%s]\n", __FUNCTION__, strFile.c_str());
    if (m_oConfig.ParseFile(strFile) != 0)
    {
        HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s, Parse config error. Error[%s]\n", __FUNCTION__, m_oConfig.GetErrorMessage().c_str());
        return false;
    }
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s true[%s]\n", __FUNCTION__, strFile.c_str());

    m_dwThreadNumbers = m_oConfig.GetParameter(CN_HTTP_THREAD, CDV_HTTP_THREAD);

    ParseHttpPorts();
	ParseOsapConfig();
    ParseReportURLs();
	ParseTransferConfig();
    ParseLocalAlias();
	ParseAccessConfig();
    ParseServerConfig();
    ParseAgentConfig();
    return true;
}


const vector<SEncoderConfig> &CHttpRecConfig::GetEncoderConfig(int dwServerNo, const string &strRelPath)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s, ServerNo[%d], Path[%s]\n", __FUNCTION__, dwServerNo, strRelPath.c_str());
    const static vector<SEncoderConfig> EMPTY_ENCODER_CONFIG;
    if ((unsigned)dwServerNo >= m_vecServerConfig.size() || dwServerNo < 0)
    {
        return EMPTY_ENCODER_CONFIG;
    }
    else
    {
        const map< string, vector<SEncoderConfig> > &mapPathAndEncoder = m_vecServerConfig[dwServerNo]._mapPathAndEncoder;
        map< string, vector<SEncoderConfig> >::const_iterator iterChoosen = mapPathAndEncoder.find("");
        if (!strRelPath.empty())
        {
            map< string, vector<SEncoderConfig> >::const_iterator iterEncoder = mapPathAndEncoder.begin();
            string strChoosenPath;
            while (iterEncoder != mapPathAndEncoder.end())
            {
                if (boost::istarts_with(strRelPath, iterEncoder->first)
                    && (iterEncoder->first.length() > strChoosenPath.length()))
                {
                    strChoosenPath = iterEncoder->first;
                    iterChoosen = iterEncoder;
                }

                ++iterEncoder;
            }
        }

        return (iterChoosen == mapPathAndEncoder.end() ? EMPTY_ENCODER_CONFIG : iterChoosen->second);
    }
}

const SAccessConfig &CHttpRecConfig::GetAccessConfig(const string &strHost)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s Host[%s]\n", __FUNCTION__, strHost.c_str());
    static const SAccessConfig INVAILD_ACCESS_CONFIG(false);

    map<string, SAccessConfig>::const_iterator iter = m_mapAccessConfig.find(strHost);
    if (iter != m_mapAccessConfig.end())
    {
        return iter->second;
    }
    else
    {
        return INVAILD_ACCESS_CONFIG;
    }
}


void CHttpRecConfig::GetReportURLs(string &strMonitorUrl, string &strDetailMonitorUrl, string &strDgsUrl, string &strErrorUrl)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);
    strMonitorUrl = m_strMonitorUrl;
    strDetailMonitorUrl = m_strDetailMonitorUrl;
    strDgsUrl = m_strDgsUrl;
    strErrorUrl = m_strErrorUrl;
}

bool CHttpRecConfig::IsLocal(const string &strHostName, unsigned int dwPort)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s HostName[%s], Port[%u]\n", __FUNCTION__, strHostName.c_str(), dwPort);
    bool bIsHostAlias = (m_vecLocalAlias.end() != std::find(m_vecLocalAlias.begin(), m_vecLocalAlias.end(), strHostName));
    bool bIsListeningPort = (m_vecHttpPorts.end() != std::find(m_vecHttpPorts.begin(), m_vecHttpPorts.end(), dwPort));
    return ((bIsHostAlias && bIsListeningPort) || strHostName.empty());
}

void CHttpRecConfig::ParseHttpPorts()
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);
    string strPorts = m_oConfig.GetParameter(CN_HTTP_PORT, CDV_HTTP_PORT);
    vector<string> vecPort;
    boost::algorithm::split(vecPort, strPorts, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
    unsigned int nSize = vecPort.size();
    for (unsigned int iIter = 0; iIter < nSize; ++iIter)
    {
        unsigned int dwPort = atoi(vecPort[iIter].c_str());
        m_vecHttpPorts.push_back(dwPort);
    }
}

void CHttpRecConfig::ParseOsapConfig()
{
	HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);
    m_bIsOsapVaild = (m_oConfig.GetParameter(CN_HTTP_OSAP_OPTION, CDV_HTTP_OSAP_OPTION) == 1);
    m_strOsapServerAddrs = m_oConfig.GetParameter(CN_HTTP_OSAP_SERVER_ADDRS, CDV_HTTP_OSAP_SERVER_ADDRS);
}

void CHttpRecConfig::ParseReportURLs()
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);
    m_strMonitorUrl = m_oConfig.GetParameter(CN_REPORT_URL, CDV_REPORT_URL);
    m_strDetailMonitorUrl = m_oConfig.GetParameter(CN_DETAIL_REPORT_URL, CDV_DETAIL_REPORT_URL);
    m_strDgsUrl = m_oConfig.GetParameter(CN_DSG_URL, CDV_DSG_URL);
    m_strErrorUrl = m_oConfig.GetParameter(CN_ERR_REPORT_URL, CDV_ERR_REPORT_URL);
}

void CHttpRecConfig::ParseTransferConfig()
{
	HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);
    m_sTransferConfig._strH2ADir = m_oConfig.GetParameter(CN_HTTP_H2A_DIR, CDV_HTTP_H2A_DIR);
    m_sTransferConfig._strH2AFilter = m_oConfig.GetParameter(CN_HTTP_H2A_FILE_REGEX, CDV_HTTP_H2A_FILE_REGEX);
    m_sTransferConfig._strA2HDir = m_oConfig.GetParameter(CN_HTTP_A2H_DIR, CDV_HTTP_A2H_DIR);
    m_sTransferConfig._strA2HFilter = m_oConfig.GetParameter(CN_HTTP_A2H_FILE_REGEX, CDV_HTTP_A2H_FILE_REGEX);
}

void CHttpRecConfig::ParseLocalAlias()
{
	HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);
    string strAlias = m_oConfig.GetParameter(CN_HTTP_LOACLHOST, CDV_HTTP_LOACLHOST);
    boost::algorithm::split(m_vecLocalAlias, strAlias, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
    unsigned int nSize = m_vecLocalAlias.size();
	for (unsigned int iIter = 0; iIter < nSize; ++iIter)
	{
        boost::algorithm::trim(m_vecLocalAlias[iIter]);
	}
}

void CHttpRecConfig::ParseAccessConfig()
{
	HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);
    vector<string> vecAccessConfig = m_oConfig.GetParameters(CN_HTTP_ACCESS);
	unsigned int nSize = vecAccessConfig.size();
	for (unsigned int iIter = 0; iIter < nSize; ++iIter)
	{
		vector<string> vecConfigItem;
        boost::algorithm::split(vecConfigItem, vecAccessConfig[iIter], boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
		if (vecConfigItem.size() != 3)
		{
            HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s Incorrect access configure. [%s]\n", __FUNCTION__, vecAccessConfig[iIter].c_str());
			continue;
		}

		SAccessConfig sAccessConfig;
        sAccessConfig._bIsVaild = true;
		sAccessConfig._strHost = vecConfigItem[0];
		sAccessConfig._strMerchantName = vecConfigItem[1];
		sAccessConfig._strPrivateKey = vecConfigItem[2];

		m_mapAccessConfig[sAccessConfig._strHost] = sAccessConfig;
	}
}

void CHttpRecConfig::ParseServerConfig()
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);

    vector<string> vecServerConfig = m_oConfig.GetParameters("HTTP/Server");
    unsigned int dwServerNo = 0;
    for (vector<string>::const_iterator iterServer = vecServerConfig.begin();
         iterServer != vecServerConfig.end();
         ++iterServer)
    {
        ParseOneServerConfig(*iterServer, dwServerNo);
        ++dwServerNo;
    }
}

void CHttpRecConfig::ParseAgentConfig()
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);
    vector<string> vecAgentConfig = m_oConfig.GetParameters("HTTP/Agent");
    for (vector<string>::const_iterator iterAgent = vecAgentConfig.begin();
        iterAgent != vecAgentConfig.end();
        ++iterAgent)
    {
        ParseOneAgentConfig(*iterAgent);
    }
}

bool CHttpRecConfig::ParseOneServerConfig(const string &strConfig, unsigned int dwServerNo)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s Conifg[%s], ServerNo[%u]\n", __FUNCTION__, strConfig.c_str(), dwServerNo);

    sdo::common::CXmlConfigParser oServerConfig;
    if (0 == oServerConfig.ParseDetailBuffer(strConfig.c_str()))
    {
        SServerConfig sServerConfig;

        /////////
        string strContexts = oServerConfig.GetParameter("Context");
        vector<string> vecContext;
        boost::algorithm::split(vecContext, strContexts, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
        for (vector<string>::const_iterator iterContext = vecContext.begin();
            iterContext != vecContext.end();
            ++iterContext)
        {
            boost::smatch sMatch;
            if (boost::regex_search(*iterContext, sMatch, boost::regex("([^/]+)/?([\\s\\S]*)")))
            {
                SServerContext sServerContext;
                sServerContext._strDomain = boost::trim_copy(sMatch[1].str());
                sServerContext._strPath = boost::trim_copy(sMatch[2].str());
                sServerConfig._vecContext.push_back(sServerContext);
            }
        }

        /////////
        string strRootPath = oServerConfig.GetParameter("Location");
        if (!boost::iends_with(strRootPath, "/"))
        {
            strRootPath += '/';
        }
        sServerConfig._dwServerNo = dwServerNo;
        sServerConfig._sFileManagerConfig._strRootPath = boost::filesystem::system_complete(strRootPath).string();
        sServerConfig._sFileManagerConfig._strDefaultFile = oServerConfig.GetParameter("Default");
        sServerConfig._sFileManagerConfig._strDefaultExtension = oServerConfig.GetParameter("DefaultExtension", "html");

        ///////
        string strUploadPath = oServerConfig.GetParameter("UploadPath");
        if (!boost::iends_with(strUploadPath, "/"))
        {
            strUploadPath += '/';
        }
        sServerConfig._sUploadConfig._strUploadPath = boost::filesystem::system_complete(strUploadPath).string();
        sServerConfig._sUploadConfig._strUploadURL = oServerConfig.GetParameter("UploadURL");

        /////////
        vector<string> vecPathConfig = oServerConfig.GetParameters("PathConfig");
        for (vector<string>::const_iterator iterPath = vecPathConfig.begin();
            iterPath != vecPathConfig.end();
            ++iterPath)
        {
            vector<SEncoderConfig> vecEncoder;
            string strPath;
            if (ParseOnePathConfig(*iterPath, strPath, vecEncoder))
            {
                sServerConfig._mapPathAndEncoder[strPath] = vecEncoder;
            }
        }

        m_vecServerConfig.push_back(sServerConfig);
        return true;
    }
    else
    {
        HTTP_REC_XLOG(XLOG_WARNING, "CHttpRecConfig::%s Conifg[%s], Can't parse configure.\n", __FUNCTION__, strConfig.c_str());
        return false;
    }
}

bool CHttpRecConfig::ParseOnePathConfig(const string &strConfig, string &strPath, vector<SEncoderConfig> &vecEncoder)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s Conifg[%s]\n", __FUNCTION__, strConfig.c_str());
    sdo::common::CXmlConfigParser oPathConfig;
    if (0 == oPathConfig.ParseDetailBuffer(strConfig.c_str()))
    {
        strPath = oPathConfig.GetParameter("Path");
        if (!boost::istarts_with(strPath, "/"))
        {
            HTTP_REC_XLOG(XLOG_WARNING, "CHttpRecConfig::%s Conifg[%s], The path don't begin with '/'\n", __FUNCTION__, strConfig.c_str());
            return false;
        }
        strPath.erase(0);

        vector<string> vecEncoderConfig = oPathConfig.GetParameters("Encoder");
        for (vector<string>::const_iterator iterEncoder = vecEncoderConfig.begin();
            iterEncoder != vecEncoderConfig.end();
            ++iterEncoder)
        {
            boost::smatch sMatch;
            if (boost::regex_search(*iterEncoder, sMatch, boost::regex("([01]),([^,]+),([\\s\\S]*)")))
            {
                SEncoderConfig sEncoderConfig;
                sEncoderConfig.bExactMatch = boost::iequals(sMatch[1].str(), "0");
                sEncoderConfig._strPattern = boost::regex_replace(sMatch[2].str(), boost::regex("&quot;"), "\"", boost::match_default | boost::format_all);
                sEncoderConfig._strReplace = boost::regex_replace(sMatch[3].str(), boost::regex("&quot;"), "\"", boost::match_default | boost::format_all);;
                vecEncoder.push_back(sEncoderConfig);
            }
        }
        return true;
    }
    else
    {
        HTTP_REC_XLOG(XLOG_WARNING, "CHttpRecConfig::%s Conifg[%s], Can't parse configure.\n", __FUNCTION__, strConfig.c_str());
        return false;
    }
}


bool CHttpRecConfig::ParseOneAgentConfig(const string &strConfig)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s Conifg[%s]\n", __FUNCTION__, strConfig.c_str());
    sdo::common::CXmlConfigParser oServerConfig;
    if (0 == oServerConfig.ParseDetailBuffer(strConfig.c_str()))
    {
        SAgentConfig sAgentConfig;

        //parse path
        sAgentConfig._strPath = oServerConfig.GetParameter("Path");
        if (!boost::iends_with(sAgentConfig._strPath, "/"))
        {
            sAgentConfig._strPath += "/";
        }
        if (boost::istarts_with(sAgentConfig._strPath, "/"))
        {
            sAgentConfig._strPath = sAgentConfig._strPath.substr(1);
        }
        
        //parse target domain 
        SAddrPortPair sAddrPortPair;
        ParseAddrPort(oServerConfig.GetParameter("TargetDomain"), sAddrPortPair);
        if (sAddrPortPair._strAddr.empty())
        {
            HTTP_REC_XLOG(XLOG_WARNING, "CHttpRecConfig::%s, Empty 'TargetDomain' configure.\n", __FUNCTION__);
            return false;
        }
        sAgentConfig._sTargetDomain = sAddrPortPair;
        sAgentConfig._strHostHeaderValue = sAddrPortPair._strAddr + ":" + boost::lexical_cast<string>(sAddrPortPair._dwPort);

        //parse target IPs 
        string strTargetIPs = oServerConfig.GetParameter("TargetIPs");
        vector<string> vecTargetIP;
        boost::algorithm::split(vecTargetIP, strTargetIPs, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
        for (vector<string>::const_iterator iterIP = vecTargetIP.begin();
            iterIP != vecTargetIP.end(); ++iterIP)
        {
            if (ParseAddrPort(*iterIP, sAddrPortPair))
            {
                sAgentConfig._vecTargetIp.push_back(sAddrPortPair);
            }
        }
        m_vecAgentConfig.push_back(sAgentConfig);
        return true;
    }
    else
    {
        HTTP_REC_XLOG(XLOG_WARNING, "CHttpRecConfig::%s Conifg[%s], Can't parse configure.\n", __FUNCTION__, strConfig.c_str());
        return false;
    }
}

bool CHttpRecConfig::ParseAddrPort(const string &strAddrPort, SAddrPortPair &sAddrPortPair)
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s AddrPort[%s]\n", __FUNCTION__, strAddrPort.c_str());
    bool bRet = false;
    boost::smatch sMatch;
    if (boost::regex_search(strAddrPort, sMatch, boost::regex("([^:]+):?([0-9]*)")))
    {
        sAddrPortPair._strAddr = sMatch[1].str();
        try
        {
            sAddrPortPair._dwPort = boost::lexical_cast<unsigned int>(sMatch[2].str());
        }
        catch (std::exception &e)
        {
            sAddrPortPair._dwPort = 0;
            HTTP_REC_XLOG(XLOG_WARNING, "CHttpRecConfig::%s, Wrong port.\n", __FUNCTION__, e.what());
        }
        
        if (0 == sAddrPortPair._dwPort)
        {
            sAddrPortPair._dwPort = 80;
        }
        bRet = true;
    }
    
    return bRet;
}

void CHttpRecConfig::ParseWhiteValidateListConfig()
{
    HTTP_REC_XLOG(XLOG_DEBUG, "CHttpRecConfig::%s\n", __FUNCTION__);
    m_vecWhiteValidateList = m_oConfig.GetParameters(CN_WHITE_VALIDATE_LIST);
}

