#include "UrlAddr.h"
#include <algorithm>

namespace sdo{
namespace common{

bool CompareByExpireReversely(const SUrlAddr& urlAddr1, const SUrlAddr& urlAddr2)
{
	return (urlAddr2.expire < urlAddr1.expire);
}

CDnsVector::CDnsVector(const string &urlTag)
: m_strUrlTag(urlTag)
, m_nIndex(0)
{
	
}

CDnsVector::~CDnsVector()
{

}

int CDnsVector::PushBack(SUrlAddr &urlAddr)
{
	if (urlAddr.url != m_strUrlTag)
		return -1;

	for (DnsVector::iterator itr = m_DnsVector.begin(); itr != m_DnsVector.end(); itr++)
	{
		if (itr->addr == urlAddr.addr)
		{
			DoUpdate(*itr, urlAddr);
			return 0;
		}
	}

	DoInsert(urlAddr);
	return 0;
}

int CDnsVector::GetFrontElement(SUrlAddr &urlAddr)
{
	if (m_DnsVector.empty())
		return -1;

	urlAddr = m_DnsVector.front();
	return 0;
}

int CDnsVector::GetHistoryElement(SUrlAddr &urlAddr)
{
	if (m_nIndex >= m_DnsVector.size())
	{
		urlAddr = m_DnsVector[m_nIndex++];
		return 0;
	}
	return -1;
}

void CDnsVector::DoUpdate(SUrlAddr &urlAddr1, SUrlAddr &urlAddr2)
{
	if (urlAddr1.expire < urlAddr2.expire)
	{
		urlAddr1.expire = urlAddr2.expire;	//update expire
		std::sort(m_DnsVector.begin(), m_DnsVector.end(), CompareByExpireReversely);
	}
}

void CDnsVector::DoInsert(SUrlAddr &urlAddr)
{
	m_DnsVector.push_back(urlAddr);
	std::sort(m_DnsVector.begin(), m_DnsVector.end(), CompareByExpireReversely);
	
	if (m_DnsVector.size() > 10)
		m_DnsVector.pop_back();

}

void CDnsVector::Dump()
{
	//for (DnsVector::iterator itr = m_DnsVector.begin(); itr != m_DnsVector.end(); itr++)
	//{
	//	SUrlAddr &urlAddr = *itr;
	//}
}

}
}

