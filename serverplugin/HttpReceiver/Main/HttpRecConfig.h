#ifndef _HTTP_REC_CONFIG_H_
#define _HTTP_REC_CONFIG_H_

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

//#include "AsyncVirtualClientLog.h"
#include "XmlConfigParser.h"

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;
using std::pair;

typedef struct stAccessConfig
{
    bool _bIsVaild;
	string _strHost;
	string _strMerchantName;
	string _strPrivateKey;
    stAccessConfig() : _bIsVaild(false){}
    stAccessConfig(bool bIsVaild) : _bIsVaild(bIsVaild){}
}SAccessConfig;

typedef struct stEncoderConfig
{
    bool bExactMatch;
    string _strPattern;
    string _strReplace;
}SEncoderConfig;

typedef struct stServerContext
{
    string _strDomain;
    string _strPath;
}SServerContext;

typedef struct stFileManagerConfig
{
    string _strRootPath;
    string _strDefaultFile;
    string _strDefaultExtension;
}SFileManagerConfig;

typedef struct stUploadConfig
{
    string _strUploadPath;
    string _strUploadURL;
}SUploadConfig;

typedef struct stServerConfig
{
    int _dwServerNo;
    vector<SServerContext> _vecContext;
    SFileManagerConfig _sFileManagerConfig;
    SUploadConfig _sUploadConfig;
    map< string, vector<SEncoderConfig> > _mapPathAndEncoder;
}SServerConfig;

typedef struct stTransferConfig
{
	string _strH2ADir;
	string _strH2AFilter;
	string _strA2HDir;
	string _strA2HFilter;
}STransferConfig;

typedef struct stAddrPortPair
{
    string _strAddr;
    unsigned int _dwPort;
    stAddrPortPair() : _dwPort(0) {}
}SAddrPortPair;

typedef struct stAgentConfig
{
    string _strPath;
    string _strHostHeaderValue;
    SAddrPortPair _sTargetDomain;
    vector<SAddrPortPair> _vecTargetIp; 
    unsigned int _dwNextIpIndex;
    stAgentConfig() : _dwNextIpIndex(0) {}
}SAgentConfig;

class CHttpRecConfig;
typedef boost::shared_ptr<CHttpRecConfig> HttpRecConfigPtr;

class CHttpRecConfig : public boost::enable_shared_from_this<CHttpRecConfig>,
	                   private boost::noncopyable
{
public:
    static HttpRecConfigPtr GetInstance();
    virtual ~CHttpRecConfig();
    bool Load(const string strFile);

    vector<string> &GetLocalAlias() { return m_vecLocalAlias; }
    vector<unsigned int> &GetHttpPorts() { return m_vecHttpPorts; }
    unsigned int GetThreadNumbers() { return m_dwThreadNumbers; }
	const string &GetOsapServerAddrs() { return m_strOsapServerAddrs; }
	const STransferConfig &GetTransferConfig() { return m_sTransferConfig; }
    const vector<SServerConfig> &GetServerConfig() { return m_vecServerConfig; }
    const vector<SEncoderConfig> &GetEncoderConfig(int dwServerNo, const string &strAbsPath);
    const SAccessConfig &GetAccessConfig(const string &strHost);
    const vector<SAgentConfig> &GetAgentConfig() { return m_vecAgentConfig; }
    void GetReportURLs(string &strMonitorUrl, string &strDetailMonitorUrl, string &strDgsUrl, string &strErrorUrl);
    const vector<string> &GetWhiteValidateList() { return m_vecWhiteValidateList; }

    bool IsOsapVaild() { return m_bIsOsapVaild; }
    bool IsLocal(const string &strHostName, unsigned int dwPort);

private:
    void ParseHttpPorts();
	void ParseOsapConfig();
    void ParseReportURLs();
	void ParseTransferConfig();
	void ParseLocalAlias();
	void ParseAccessConfig();
    void ParseServerConfig();
    void ParseAgentConfig();
    void ParseWhiteValidateListConfig();

private:
    bool ParseOneServerConfig(const string &strConfig, unsigned int dwServerNo);
    bool ParseOnePathConfig(const string &strConfig, string &strPath, vector<SEncoderConfig> &sServerConfig);
    bool ParseOneAgentConfig(const string &strConfig);
    bool ParseAddrPort(const string &strAddrPort, SAddrPortPair &sAddrPortPair);

private:
    static HttpRecConfigPtr s_pInstance;

    string m_strFile;
	sdo::common::CXmlConfigParser m_oConfig;
 
    bool m_bIsOsapVaild;
    string m_strOsapServerAddrs;

	STransferConfig m_sTransferConfig;

    string m_strRemoteHost;
    unsigned int m_dwRemotePort;

    string m_strMonitorUrl;
    string m_strDetailMonitorUrl;
    string m_strDgsUrl;
    string m_strErrorUrl;

	unsigned int m_dwThreadNumbers;
	vector<unsigned int> m_vecHttpPorts;
	vector<string> m_vecLocalAlias;
    map<string, SAccessConfig> m_mapAccessConfig;
    vector<SServerConfig> m_vecServerConfig;

    vector<SAgentConfig> m_vecAgentConfig;
    vector<string> m_vecWhiteValidateList;
};

#endif //_HTTP_REC_CONFIG_H_
