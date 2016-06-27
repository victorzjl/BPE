#include "DnsManager.h"
#include "DnsAccessor.h"

namespace sdo{
namespace common{

void TimeCalculator::GetExpire(struct timeval_a &expireTime, unsigned int nSeconds, unsigned int nMilliseconds)
{
	struct timeval_a now;
	gettimeofday_a(&now,0);

	struct timeval_a interval;
	interval.tv_sec = nSeconds;
	interval.tv_usec = nMilliseconds;

	timeradd(&now, &interval, &expireTime);
}

bool TimeCalculator::IsExpired(struct timeval_a IN &expireTime)
{
	struct timeval_a now;
	gettimeofday_a(&now,0);

	return (expireTime<now);
}

CDnsManager::CDnsManager()
{
	m_dnsMap.reset(new DnsMap);
}

CDnsManager::~CDnsManager()
{
	
}

int CDnsManager::InitialUrlAddr()
{
	DnsMap *pDnsMap = m_dnsMap.get();
	if (CDnsAccessor::Instance()->LoadDnsConfig(*pDnsMap)!=0)
		return -1;

	return 0;
}

int CDnsManager::GetUrlAddr(const string &strUrl, string &strAddr)
{
	if (strUrl.empty())
		return -1;

	DnsMap *pDnsMap = m_dnsMap.get();
	DnsMap::iterator itr = pDnsMap->find(strUrl);
	
	if (itr == pDnsMap->end())
		return -2;

	SUrlAddr urlAddr;
	CDnsVector &dnsVec = itr->second;

	if (dnsVec.GetFrontElement(urlAddr) != 0)
		return -3;

	if (TimeCalculator::IsExpired(urlAddr.expire))
		return -4;

	strAddr = urlAddr.addr;
	return 0;
}

int CDnsManager::StoreUrlAddr(const string &strUrl, const string &strAddr)
{
	if (strUrl.empty())
		return -1;

	SUrlAddr urlAddr;
	urlAddr.url = strUrl;
	urlAddr.addr = strAddr;
	TimeCalculator::GetExpire(urlAddr.expire, EXPIRE_INTERVAL, 0);

	DnsMap *pDnsMap = m_dnsMap.get();
	CDnsAccessor::Instance()->InsertToDnsMap(*pDnsMap,urlAddr);

	if (CDnsAccessor::Instance()->StoreDnsConfig(*pDnsMap)!=0)
		return -2;

	return 0;
}

int CDnsManager::GetHistoryUrlAddr(const string &strUrl, string &strAddr)
{
	if (strUrl.empty())
		return -1;

	DnsMap *pDnsMap = m_dnsMap.get();
	DnsMap::iterator itr = pDnsMap->find(strUrl);

	if (itr == pDnsMap->end())
		return -2;

	SUrlAddr urlAddr;
	CDnsVector &dnsVec = itr->second;

	if (dnsVec.GetHistoryElement(urlAddr) != 0)
		return -3;
	
	strAddr = urlAddr.addr;
	return 0;
}

}
}


