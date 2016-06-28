#ifndef _CVS_ARCHIVE_ADAPTER_H_
#define _CVS_ARCHIVE_ADAPTER_H_

#include "RedisMsg.h"

#include <vector>
#include <string>

using std::vector;
using std::string;
using namespace redis;

class CRvsArchiveAdapter
{
public:
	static string Oarchive(const vector<RVSKeyValue> &vecKeyValue);
	static int Iarchive(const string &ar, vector<RVSKeyValue>& vecKeyValue);
};
 
#endif
