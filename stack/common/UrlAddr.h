#ifndef _URL_ADDR_H_
#define _URL_ADDR_H_
#include <string>
#include <map>
#include <vector>
#include <stdlib.h>
#include "_time.h"

using std::string;
using std::map;
using std::vector;

namespace sdo{
namespace common{

struct SUrlAddr
{
	string url;
	string addr;
	struct timeval_a expire;
public:
	SUrlAddr()
	{

	}
	SUrlAddr(const string &url_, const string &addr_, const timeval_a &expire_)
		: url(url_), addr(addr_), expire(expire_)
	{
	}
	SUrlAddr(const string &url_, const string &addr_, const string &sec_, const string &usec_)
		: url(url_), addr(addr_)
	{
		expire.tv_sec = atol(sec_.c_str());
		expire.tv_usec = atol(usec_.c_str());
	}
public:
	inline bool operator<(const SUrlAddr& obj) const
	{
		return (expire < obj.expire);
	}
	inline bool operator==(const SUrlAddr& obj) const
	{
		return (expire == obj.expire);
	}
};

typedef vector<SUrlAddr> DnsVector;

class CDnsVector
{
public:
	CDnsVector(const string &urlTag);
	~CDnsVector();
public:
	int PushBack(SUrlAddr &urlAddr);
	int GetFrontElement(SUrlAddr &urlAddr);
	int GetHistoryElement(SUrlAddr &urlAddr);
	inline void Reset() { m_nIndex = 0; }
	inline DnsVector& GetDnsVector() { return m_DnsVector; }
private:
	void DoUpdate(SUrlAddr &urlAddr1, SUrlAddr &urlAddr2);
	void DoInsert(SUrlAddr &urlAddr);
public:
	void Dump();
private:
	DnsVector m_DnsVector;
	string m_strUrlTag;
	unsigned int m_nIndex;
};

typedef map<string, CDnsVector> DnsMap;

}
}

#endif


