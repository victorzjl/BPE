#ifndef _CVS_ARCHIVE_ADAPTER_H_
#define _CVS_ARCHIVE_ADAPTER_H_

#include "CacheMsg.h"

class CCvsArchiveAdapter
{
public:
	static string Oarchive(const vector<CVSKeyValue> &vecKeyValue, bool isClear);
	static int Iarchive(const string &ar, vector<CVSKeyValue>& vecKeyValue, bool isClear);
};
 
#endif
