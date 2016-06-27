#ifndef _SDO_DNS_MANAGER_H_
#define _SDO_DNS_MANAGER_H_
#include <boost/thread/tss.hpp>
#include <map>
#include <string>
#include "UrlAddr.h"

using std::multimap;
using std::map;
using std::string;

#define OUT
#define IN
#define EXPIRE_INTERVAL 300

namespace sdo{
namespace common{

class TimeCalculator
{
public:
	static void GetExpire(struct timeval_a OUT &expireTime, unsigned int IN nSeconds, unsigned int IN nMilliseconds = 0);
	static bool IsExpired(struct timeval_a IN &expireTime);
};

class CDnsManager
{
public:
	CDnsManager();
	~CDnsManager();
public:
	int InitialUrlAddr();
	int GetUrlAddr(const string &strUrl, string &strAddr);
	int StoreUrlAddr(const string &strUrl, const string &strAddr);
	int GetHistoryUrlAddr(const string &strUrl, string &strAddr);
private: 
	boost::thread_specific_ptr<DnsMap> m_dnsMap;
};

}
}

#endif

