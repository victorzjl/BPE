#include "DnsAccessor.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/algorithm/string.hpp> 

using std::ifstream;
using std::ofstream;

using boost::interprocess::file_lock;
using boost::interprocess::scoped_lock;

namespace sdo{
namespace common{

CDnsAccessor* CDnsAccessor::sm_pInstance = NULL;

CDnsAccessor* CDnsAccessor::Instance() 
{
	if (sm_pInstance == NULL)
		sm_pInstance = new CDnsAccessor;

	return sm_pInstance;
}

void CDnsAccessor::Start(const string& strDnsConfigFile)
{
	m_strDnsConfigFile = strDnsConfigFile;
}

int CDnsAccessor::LoadDnsConfig(DnsMap &dnsMap)
{
	if (m_strDnsConfigFile.empty())
		return -1;

	if (!boost::filesystem::exists(m_strDnsConfigFile))
		return -2;

	file_lock f_lock(m_strDnsConfigFile.c_str());
	scoped_lock<file_lock> s_lock(f_lock);

	return ReadDnsConfig(m_strDnsConfigFile,dnsMap);
}

int CDnsAccessor::StoreDnsConfig(DnsMap &dnsMap)
{
	if (m_strDnsConfigFile.empty())
		return -1;

	if (!boost::filesystem::exists(m_strDnsConfigFile))
		return -2;

	file_lock f_lock(m_strDnsConfigFile.c_str());
	scoped_lock<file_lock> s_lock(f_lock);

	if (ReadDnsConfig(m_strDnsConfigFile, dnsMap) != 0)
		return -3;

	if (WriteDnsConfig(m_strDnsConfigFile, dnsMap) != 0)
		return -4;

	return 0;
}

int CDnsAccessor::ReadDnsConfig(const string& strDnsConfig, DnsMap &dnsMap)
{
	if (strDnsConfig.empty())
		return -1;

	ifstream dnsCfgFile(strDnsConfig.c_str(), std::ios::in);
	if (!dnsCfgFile.is_open() || dnsCfgFile.bad())
		return -2;
	
	while (!dnsCfgFile.eof())
	{
		char szBuffer[1024] = {0};
		dnsCfgFile.getline(szBuffer,1023);
		string strDnsRecord = szBuffer;

		if (strDnsRecord.empty())
			continue;

		vector<string> vecItems;
		boost::algorithm::split(vecItems, strDnsRecord, boost::algorithm::is_any_of("\t"), boost::algorithm::token_compress_on);

		if (vecItems.size()!=4)
			continue;

		SUrlAddr urlAddr(vecItems[0], vecItems[1], vecItems[2], vecItems[3]);
		InsertToDnsMap(dnsMap, urlAddr);
	}

	dnsCfgFile.close();
	return 0;
}

int CDnsAccessor::WriteDnsConfig(const string& strDnsConfig, DnsMap &dnsMap)
{
	if (strDnsConfig.empty())
		return -1;

	ofstream dnsCfgFile(strDnsConfig.c_str(), std::ios::out|std::ios::trunc); 
	if (!dnsCfgFile.is_open() || dnsCfgFile.bad())
		return -2;

	for (DnsMap::iterator itr = dnsMap.begin(); itr != dnsMap.end(); itr++)
	{
		DnsVector& dnsVec = itr->second.GetDnsVector();
		for (DnsVector::iterator itrVec = dnsVec.begin(); itrVec != dnsVec.end(); itrVec++)
		{
			const SUrlAddr& urlAddr = *itrVec;

			char szBuffer[1024] = {0};
			snprintf(szBuffer,sizeof(szBuffer)-1,"%s\t%s\t%ld\t%ld\n", urlAddr.url.c_str(), urlAddr.addr.c_str(), urlAddr.expire.tv_sec, urlAddr.expire.tv_usec);
			dnsCfgFile.write(szBuffer,strlen(szBuffer));
		}
	}

	dnsCfgFile.flush();
	dnsCfgFile.close();
	return 0;
}

void CDnsAccessor::InsertToDnsMap(DnsMap &dnsMap, SUrlAddr &urlAddr)
{
	if (dnsMap.count(urlAddr.url) == 0)
	{
		CDnsVector dnsVec(urlAddr.url);
		dnsVec.PushBack(urlAddr);
		dnsMap.insert(std::pair<string,CDnsVector>(urlAddr.url,dnsVec));
	}
	else
	{
		DnsMap::iterator itr = dnsMap.find(urlAddr.url);
		if (itr != dnsMap.end())
		{
			CDnsVector& dnsVec = itr->second;
			dnsVec.PushBack(urlAddr);
			dnsVec.Reset();
		}
	}
}

void CDnsAccessor::Dump(DnsMap &dnsMap)
{
	for (DnsMap::iterator itr = dnsMap.begin(); itr != dnsMap.end(); itr++)
	{
		CDnsVector &dnsVec = itr->second;
		dnsVec.Dump();
	}
}

}
}

