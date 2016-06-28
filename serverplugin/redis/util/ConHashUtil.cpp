#include "ConHashUtil.h"
#include "RedisVirtualServiceLog.h"

namespace redis{

CConHashUtil::CConHashUtil():m_nCurrentPos(0), conhash(NULL)
{
}

CConHashUtil::~CConHashUtil()
{
	if(conhash)
	{
		conhash_fini(conhash);
		conhash = NULL;
	}
}

bool CConHashUtil::Init()
{
	RVS_XLOG(XLOG_DEBUG, "CConHashUtil::%s\n", __FUNCTION__);
	conhash = conhash_init(NULL);
	return conhash ? true : false;
}

bool CConHashUtil::AddNode( const string &strNodeInfo, unsigned int dwReplica )
{
	RVS_XLOG(XLOG_DEBUG, "CConHashUtil::%s\n", __FUNCTION__);
	dwReplica = dwReplica <= 0 ? 1 : dwReplica;
	map<string, int>::iterator citr = m_nodeIndexByName.find(strNodeInfo);
	if(citr!=m_nodeIndexByName.end())
	{
		conhash_set_node(&m_node[citr->second], strNodeInfo.c_str(), dwReplica);
		return conhash_add_node(conhash, &m_node[citr->second])==0 ? true : false;
	}
	else
	{
		if(m_nCurrentPos >= MAX_NODE_NUM)
		{
			RVS_XLOG(XLOG_ERROR, "CConHashUtil::%s, too much node\n", __FUNCTION__);
			return false;
		}

		m_nodeIndexByName.insert(make_pair(strNodeInfo, m_nCurrentPos));
		conhash_set_node(&m_node[m_nCurrentPos], strNodeInfo.c_str(), dwReplica);
		int nRet = conhash_add_node(conhash, &m_node[m_nCurrentPos]);
		++m_nCurrentPos;
		
		return nRet==0 ? true : false;
	}
}

bool CConHashUtil::DelNode( const string &strNodeInfo )
{
	RVS_XLOG(XLOG_DEBUG, "CConHashUtil::%s\n", __FUNCTION__);
	map<string, int>::iterator citr = m_nodeIndexByName.find(strNodeInfo);
	if(citr!=m_nodeIndexByName.end())
	{
		conhash_del_node(conhash, &m_node[citr->second]);
		return true;
	}
	else
	{
		return false;
	}
}

string CConHashUtil::GetConHashNode( const string &strKey )
{
	RVS_XLOG(XLOG_DEBUG, "CConHashUtil::%s\n", __FUNCTION__);
	const node_s *pnode = conhash_lookup(conhash, strKey.c_str());
	if(pnode==NULL)
	{
		return "";
	}
	else
	{
		return pnode->iden;
	}
	
}

}

