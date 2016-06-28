#ifndef _CON_HASH_UTIL_H_
#define _CON_HASH_UTIL_H_

#include "conhash.h"
#include "RedisMsg.h"
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;
using std::make_pair;

namespace redis{
	
class CConHashUtil
{
public:
	CConHashUtil();
	virtual ~CConHashUtil();
	bool Init();
	bool AddNode(const string &strNodeInfo, unsigned int dwReplica);
	bool DelNode(const string &strNodeInfo);
	string GetConHashNode( const string &strKey );
private:
	node_s m_node[MAX_NODE_NUM];
	int m_nCurrentPos;
	conhash_s *conhash;
	map<string, int> m_nodeIndexByName;
};

}
#endif
