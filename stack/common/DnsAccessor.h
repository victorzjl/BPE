#ifndef _DNS_ACCESSOR_H_
#define _DNS_ACCESSOR_H_
#include <vector>
#include <string>
#include "UrlAddr.h"

using std::vector;
using std::string;
using sdo::common::DnsMap;

namespace sdo{
namespace common{

class CDnsAccessor
{
protected:
	CDnsAccessor() {}
	~CDnsAccessor() {}
	CDnsAccessor(const CDnsAccessor&) {}
public:
	static CDnsAccessor* Instance();
	void Start(const string& strDnsConfigFile);
public:
	int LoadDnsConfig(DnsMap &dnsMap);
	int StoreDnsConfig(DnsMap &dnsMap);
	void InsertToDnsMap(DnsMap &dnsMap, SUrlAddr &urlAddr);
private:
	int ReadDnsConfig(const string& strDnsConfig, DnsMap &dnsMap);
	int WriteDnsConfig(const string& strDnsConfig, DnsMap &dnsMap);
	void Dump(DnsMap &dnsMap);
private:
	static CDnsAccessor* sm_pInstance;
	string m_strDnsConfigFile;
};

}
}

#endif

